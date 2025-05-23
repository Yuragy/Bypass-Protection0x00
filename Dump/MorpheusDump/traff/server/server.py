#!/usr/bin/env python3
import socket
import struct
import sys
import logging
import time

# -----------------------------
# ПАРАМЕТРЫ, согласованные с клиентом (dumper.exe без сжатия):
# -----------------------------
BLOCK_SIZE = 10              # Количество фрагментов данных на блок FEC
FRAGMENT_SIZE = 2            # По 2 байта полезной нагрузки на фрагмент
RC4_KEY = b"MySecretKey"     # Должен совпадать с кодом на C
BASE_TIMEOUT = 120           # Общее время ожидания (сек)
SOCKET_RCVBUF_SIZE = 1 << 20 # 1 МБ буфера приёмного сокета

# Логирование
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s [%(levelname)s] %(message)s'
)

# -----------------------------
# Инициализация GF(256) для RS
# -----------------------------
GF_EXP = [0]*512
GF_LOG = [0]*256

def init_gf():
    x = 1
    for i in range(255):
        GF_EXP[i] = x
        GF_LOG[x] = i
        x <<= 1
        if x & 0x100:
            x ^= 0x11d
    for i in range(255, 512):
        GF_EXP[i] = GF_EXP[i - 255]

def gf_mul(a, b):
    if a == 0 or b == 0:
        return 0
    return GF_EXP[(GF_LOG[a] + GF_LOG[b]) % 255]

def gf_inv(a):
    if a == 0:
        raise ZeroDivisionError("Inverse of 0 in GF(256)")
    # a^(255-1) = a^254 есть обратный элемент
    return GF_EXP[255 - GF_LOG[a]]

# -----------------------------
# RC4
# -----------------------------
def rc4_init(key: bytes):
    """Инициализирует массив S для RC4 по ключу."""
    S = list(range(256))
    j = 0
    for i in range(256):
        j = (j + S[i] + key[i % len(key)]) % 256
        S[i], S[j] = S[j], S[i]
    return S

def rc4_stream(S, length: int):
    """Генерирует RC4-кеystream заданной длины."""
    i = 0
    j = 0
    out = []
    for _ in range(length):
        i = (i + 1) % 256
        j = (j + S[i]) % 256
        S[i], S[j] = S[j], S[i]
        out.append(S[(S[i] + S[j]) % 256])
    return out

def rc4_decrypt(ciphertext: bytes, key: bytes, skip: int) -> bytes:
    """RC4-дешифрование с пропуском skip байт в начале."""
    S = rc4_init(key)
    # пропускаем skip
    _ = rc4_stream(S, skip)
    # на нужную длину
    ks = rc4_stream(S, len(ciphertext))
    return bytes([c ^ k for c, k in zip(ciphertext, ks)])

# -----------------------------
# Расшифровка 4 байт (seq/frag) c подбором skip
# -----------------------------
def try_decrypt(encrypted: bytes, key: bytes, skip: int):
    """Пробует расшифровать 4 байта, вернуть (val, расшифрованный-блок)."""
    decrypted = rc4_decrypt(encrypted, key, skip)
    # Предполагаем big-endian >I
    val = struct.unpack(">I", decrypted)[0]
    return val

def deduce_skip(encrypted: bytes, key: bytes, total_frags: int, is_fec: bool):
    """
    Подбирает skip (0..255), чтобы расшифрованное значение походило
    либо на обычный фрагмент (seq < total_frags),
    либо на FEC (seq >= total_frags).
    Возвращает (skip, val) либо None.
    """
    for skip in range(256):
        val = try_decrypt(encrypted, key, skip)
        seq = val >> 16
        if not is_fec:
            # Обычный фрагмент: seq < total_frags
            if seq < total_frags:
                return skip, val
        else:
            # FEC: seq >= total_frags (по логике клиента)
            if seq >= total_frags:
                return skip, val
    return None, None

# -----------------------------
# Простейшее RS-решение (Гаусс по GF(256))
# -----------------------------
def rs_solve(equations, k):
    """
    equations: список (row[], y), row длиной k
    Возвращает массив из k решений (pos0..pos(k-1)).
    """
    # Копия массива (A|b)
    A = []
    for (row, y) in equations:
        # row + [y] → единый
        A.append(row[:] + [y])

    n = len(equations)
    for col in range(k):
        # Поиск pivot
        pivot_row = None
        for row_i in range(col, n):
            if A[row_i][col] != 0:
                pivot_row = row_i
                break
        if pivot_row is None:
            raise ValueError("RS: not enough independent equations (singular)")

        # Меняем местами pivot_row и текущую
        A[col], A[pivot_row] = A[pivot_row], A[col]

        # Нормируем
        inv_val = gf_inv(A[col][col])
        for j in range(col, k+1):
            A[col][j] = gf_mul(A[col][j], inv_val)
        # Обнуление во всех остальных строках
        for r in range(n):
            if r != col and A[r][col] != 0:
                factor = A[r][col]
                for j in range(col, k+1):
                    A[r][j] ^= gf_mul(factor, A[col][j])

    # Теперь решение
    solution = [A[i][k] for i in range(k)]
    return solution

def build_equations(block_data: dict, fec_data: dict, k_block: int, pos: int):
    """
    Формируем список (row, y) для RS-решения:
    - row – длиной k_block
    - y – значение
    """
    eqs = []
    # Сначала те, что уже известны (i in block_data)
    for i, val2 in block_data.items():
        row = [0]*k_block
        row[i] = 1
        y = val2[pos]
        eqs.append((row, y))
    # Потом FEC
    # fec_data[j] = байты [2], где parity0, parity1
    for j, val2 in fec_data.items():
        # Генератор Vandermonde: alpha^(i*(j+1)) => GF_EXP[..]
        row = []
        for i in range(k_block):
            row.append(GF_EXP[(i*(j+1)) % 255])
        y = val2[pos]
        eqs.append((row, y))
    return eqs

def rs_decode_block(block_data, fec_data, k_block: int):
    """
    block_data: { i -> [2 байта], ... } (только часть i)
    fec_data:   { j -> [2 байта], ... }
    Возвращает recovered: { i -> [2 байта] } для пропущенных i
    """
    # Решаем для pos=0
    eq0 = build_equations(block_data, fec_data, k_block, pos=0)
    if len(eq0) < k_block:
        raise ValueError("Not enough equations for RS block decode (pos=0)")
    sol0 = rs_solve(eq0[:k_block], k_block)

    # Решаем для pos=1
    eq1 = build_equations(block_data, fec_data, k_block, pos=1)
    if len(eq1) < k_block:
        raise ValueError("Not enough equations for RS block decode (pos=1)")
    sol1 = rs_solve(eq1[:k_block], k_block)

    # Собираем, что «восстановили» (i,2байта)
    recovered = {}
    for i in range(k_block):
        if i not in block_data:
            recovered[i] = bytes([sol0[i], sol1[i]])
    return recovered

# -----------------------------
# Основная функция приёма
# -----------------------------
def run_receiver(host: str, port: int, output_file: str, base_timeout: int = 120):
    # Запуск UDP
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, SOCKET_RCVBUF_SIZE)

    # Пробуем привязаться к порту
    try:
        sock.bind((host, port))
    except Exception as e:
        logging.error(f"Error binding to port {port}: {e}")
        sys.exit(1)

    sock.settimeout(5)  # небольшой таймаут чтения
    logging.info(f"Listening on {host}:{port} ...")

    init_done = False
    total_fragments = 0
    total_size = 0

    data_packets = {}  # seq -> b'\x??\x??'
    fec_packets  = {}  # (blockIndex -> dict{ idxInBlock: b'\x??\x??' })
    start_time   = time.time()
    sender_addr  = None  # чтобы знать, куда отправлять feedback

    logging.info("Waiting for header...")

    while True:
        # Будем ждать
        try:
            data, addr = sock.recvfrom(1024)
        except socket.timeout:
            if time.time() - start_time > base_timeout:
                logging.warning("Global timeout reached, stopping reception.")
                break
            continue

        if sender_addr is None:
            sender_addr = addr

        # Каждый NTP-пакет должен быть длиной >= 48
        if len(data) < 48:
            continue

        payload = data[40:48]  # 8 байт Transmit Timestamp
        if not init_done:
            # Первые 4 байта: total_fragments (big-endian)
            total_fragments = struct.unpack(">I", payload[:4])[0]
            # Вторые 4 байта: total_size (big-endian)
            total_size = struct.unpack(">I", payload[4:8])[0]
            init_done = True
            logging.info(f"Header received: {total_fragments} fragments, {total_size} total bytes.")
            continue

        # Иначе это либо data-фрагмент, либо FEC
        encrypted4 = payload[4:8]  # байты, которые шифровались RC4
        # Сначала пытаемся считать это за обычный фрагмент
        skip, val = deduce_skip(encrypted4, RC4_KEY, total_fragments, is_fec=False)
        if skip is not None:
            seq = val >> 16
            fragVal = val & 0xFFFF
            # Запишем 2 байта
            # big-endian: (fragVal >> 8, fragVal & 0xFF)
            b1 = (fragVal >> 8) & 0xFF
            b2 = fragVal & 0xFF
            data_packets[seq] = bytes([b1, b2])
            progress = (len(data_packets) / total_fragments) * 100.0
            logging.info(f"Data fragment {seq} received. "
                         f"Progress: {len(data_packets)}/{total_fragments} ~ {progress:.1f}%")
        else:
            # Иначе пробуем трактовать как FEC
            skip, val = deduce_skip(encrypted4, RC4_KEY, total_fragments, is_fec=True)
            if skip is not None:
                # fec_seq = val
                seq_high = val >> 16  # blockIndex * BLOCK_SIZE + j
                fec_val  = val & 0xFFFF
                block_index = seq_high // BLOCK_SIZE
                idx_in_block= seq_high %  BLOCK_SIZE
                # Сохраним 2 байта
                b1 = (fec_val >> 8) & 0xFF
                b2 = fec_val & 0xFF
                # В словаре fec_packets: fec_packets[block_index][idx_in_block] = ...
                if block_index not in fec_packets:
                    fec_packets[block_index] = {}
                fec_packets[block_index][idx_in_block] = bytes([b1, b2])
                logging.info(f"FEC packet for block={block_index}, index={idx_in_block} received.")
            else:
                logging.warning("Could not deduce skip for a packet; ignoring.")

        # Проверяем, не все ли фрагменты мы уже получили
        if len(data_packets) == total_fragments:
            logging.info("All data fragments have been received.")
            break

    sock.close()
    logging.info(f"Reception finished: {len(data_packets)}/{total_fragments} fragments in memory.")

    if not init_done or total_fragments == 0:
        logging.warning("No valid header or zero fragments – nothing to reconstruct.")
        return

    # -----------------------------
    # Восстановление (FEC) + сбор в итоговый буфер
    # -----------------------------
    reconstructed = bytearray(total_fragments * FRAGMENT_SIZE)
    num_blocks = (total_fragments + BLOCK_SIZE - 1) // BLOCK_SIZE
    logging.info("Reconstructing dump...")

    for b in range(num_blocks):
        block_start = b * BLOCK_SIZE
        k_block     = min(BLOCK_SIZE, total_fragments - block_start)

        # block_data: i -> (два байта)
        block_data = {}
        for i in range(k_block):
            seq = block_start + i
            if seq in data_packets:
                block_data[i] = data_packets[seq]

        # fec_data: j -> (два байта), если есть
        block_fec = {}
        if b in fec_packets:
            for j, val2 in fec_packets[b].items():
                # j – индекс в блоке
                if j < BLOCK_SIZE:
                    block_fec[j] = val2

        # Если не все фрагменты есть, пробуем RS-декод
        if len(block_data) < k_block:
            try:
                recovered = rs_decode_block(block_data, block_fec, k_block)
                # recovered: { i -> [2байта] }
                for i, val2 in recovered.items():
                    block_data[i] = val2
                    missing_seq = block_start + i
                    logging.info(f"Recovered missing fragment {missing_seq} via FEC (block={b}).")
            except Exception as e:
                logging.error(f"FEC decode failed for block={b}: {e}")

        # Записываем в итоговый буфер
        for i in range(k_block):
            seq = block_start + i
            chunk = block_data.get(i, b'\x00\x00')
            offset = seq * FRAGMENT_SIZE
            reconstructed[offset:offset+2] = chunk

        # Прогресс бар/лог
        done_blocks = b + 1
        progress = (done_blocks / num_blocks) * 100.0
        logging.info(f"Block {b} done. Overall reconstruction: {progress:.1f}%")

    # Обрезаем по total_size
    reconstructed = reconstructed[:total_size]

    # -----------------------------
    # Пишем файл (без декомпрессии!)
    # -----------------------------
    with open(output_file, "wb") as f:
        f.write(reconstructed)
    logging.info(f"Final dump (raw) saved to '{output_file}'.")

    # -----------------------------
    # Если что-то не дошло, шлём feedback
    # -----------------------------
    missing_frags = []
    for i in range(total_fragments):
        if i not in data_packets:
            missing_frags.append(i)

    if missing_frags and sender_addr:
        logging.info(f"Still missing fragments after FEC: {missing_frags}")
        feedback_port = 124  # клиент слушает на 123, но feedback может быть на 124
        fb_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        # Формируем пакет feedback: [4 байта count] + затем count*4 байт номеров
        msg = struct.pack(">I", len(missing_frags))
        for seq in missing_frags:
            msg += struct.pack(">I", seq)
        try:
            fb_sock.sendto(msg, (sender_addr[0], feedback_port))
            logging.info(f"Feedback sent to {sender_addr[0]}:{feedback_port}")
        except Exception as e:
            logging.error(f"Error sending feedback: {e}")
        finally:
            fb_sock.close()

# -----------------------------
# Запуск «серверной» логики
# -----------------------------
if __name__ == "__main__":
    init_gf()  # инициализируем GF(256)

    # Настройки
    HOST = "0.0.0.0"      # слушать на всех интерфейсах
    PORT = 123            # порт UDP (NTP-like); требует привилегий на некоторых ОС
    OUTPUT_FILE = "dump_memory.bin"

    run_receiver(HOST, PORT, OUTPUT_FILE, base_timeout=BASE_TIMEOUT)
