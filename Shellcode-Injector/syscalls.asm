.code

WVM PROC
	mov rax, gs:[60h]                          ; Load PEB into RAX.
WVM_Check_X_X_XXXX:							   ; Check major version.
	cmp dword ptr [rax+118h], 10
	je  WVM_Check_10_0_XXXX
	jmp WVM_SystemCall_Unknown
WVM_Check_10_0_XXXX:						   ; Check build number for Windows 10.
	cmp word ptr [rax+120h], 10240
	je  WVM_SystemCall_10_0_10240
	cmp word ptr [rax+120h], 10586
	je  WVM_SystemCall_10_0_10586
	cmp word ptr [rax+120h], 14393
	je  WVM_SystemCall_10_0_14393
	cmp word ptr [rax+120h], 15063
	je  WVM_SystemCall_10_0_15063
	cmp word ptr [rax+120h], 16299
	je  WVM_SystemCall_10_0_16299
	cmp word ptr [rax+120h], 17134
	je  WVM_SystemCall_10_0_17134
	cmp word ptr [rax+120h], 17763
	je  WVM_SystemCall_10_0_17763
	cmp word ptr [rax+120h], 18362
	je  WVM_SystemCall_10_0_18362
	cmp word ptr [rax+120h], 18363
	je  WVM_SystemCall_10_0_18363
	cmp word ptr [rax+120h], 19041
	je  WVM_SystemCall_10_0_19041
	cmp word ptr [rax+120h], 19042
	je  WVM_SystemCall_10_0_19042
	cmp word ptr [rax+120h], 19043
	je  WVM_SystemCall_10_0_19043
	cmp word ptr [rax+120h], 19044
	je  WVM_SystemCall_10_0_19044
	cmp word ptr [rax+120h], 19045
	je  WVM_SystemCall_10_0_19045
	cmp word ptr [rax+120h], 22000			   ; START OF WINDOWS 11
	je  WVM_SystemCall_10_0_22000
	cmp word ptr [rax+120h], 22621
	je  WVM_SystemCall_10_0_22621
	cmp word ptr [rax+120h], 22631
	je  WVM_SystemCall_10_0_22631
	jmp WVM_SystemCall_Unknown
WVM_SystemCall_10_0_10240:        ; Windows 10.0.10240 (1507)
	mov eax, 003ah
	jmp WVM_Epilogue
WVM_SystemCall_10_0_10586:        ; Windows 10.0.10586 (1511)
	mov eax, 003ah
	jmp WVM_Epilogue
WVM_SystemCall_10_0_14393:        ; Windows 10.0.14393 (1607)
	mov eax, 003ah
	jmp WVM_Epilogue
WVM_SystemCall_10_0_15063:        ; Windows 10.0.15063 (1703)
	mov eax, 003ah
	jmp WVM_Epilogue
WVM_SystemCall_10_0_16299:        ; Windows 10.0.16299 (1709)
	mov eax, 003ah
	jmp WVM_Epilogue
WVM_SystemCall_10_0_17134:        ; Windows 10.0.17134 (1803)
	mov eax, 003ah
	jmp WVM_Epilogue
WVM_SystemCall_10_0_17763:        ; Windows 10.0.17763 (1809)
	mov eax, 003ah
	jmp WVM_Epilogue
WVM_SystemCall_10_0_18362:        ; Windows 10.0.18362 (1903)
	mov eax, 003ah
	jmp WVM_Epilogue
WVM_SystemCall_10_0_18363:        ; Windows 10.0.18363 (1909)
	mov eax, 003ah
	jmp WVM_Epilogue
WVM_SystemCall_10_0_19041:        ; Windows 10.0.19041 (2004)
	mov eax, 003ah
	jmp WVM_Epilogue
WVM_SystemCall_10_0_19042:        ; Windows 10.0.19042 (20H2)
	mov eax, 003ah
	jmp WVM_Epilogue
WVM_SystemCall_10_0_19043:        ; Windows 10.0.19043 (21H1)
	mov eax, 003ah
	jmp WVM_Epilogue
WVM_SystemCall_10_0_19044:        ; Windows 10.0.19044 (21H2)
	mov eax, 003ah
	jmp WVM_Epilogue
WVM_SystemCall_10_0_19045:        ; Windows 10.0.19045 (22H2)
	mov eax, 003ah
	jmp WVM_Epilogue
WVM_SystemCall_10_0_22000:        ; Windows 10.0.22000 (21H2 WIN 11)
	mov eax, 003ah
	jmp WVM_Epilogue
WVM_SystemCall_10_0_22621:        ; Windows 10.0.22621 (22H2 WIN 11) 
	mov eax, 003ah
	jmp WVM_Epilogue
WVM_SystemCall_10_0_22631:        ; Windows 10.0.22631 (23H2 WIN 11)
	mov eax, 003ah
	jmp WVM_Epilogue
WVM_SystemCall_Unknown:           ; Unknown/unsupported version.
	ret
WVM_Epilogue:
	mov r10, rcx
	int 2Eh
	ret
WVM ENDP

AVM PROC
	mov rax, gs:[60h]                             ; Load PEB into RAX.
AVM_Check_X_X_XXXX:								  ; Check major version.
	cmp dword ptr [rax+118h], 10
	je  AVM_Check_10_0_XXXX
	jmp AVM_SystemCall_Unknown
AVM_Check_10_0_XXXX:							  ; Check build number for Windows 10.
	cmp word ptr [rax+120h], 10240
	je  AVM_SystemCall_10_0_10240
	cmp word ptr [rax+120h], 10586
	je  AVM_SystemCall_10_0_10586
	cmp word ptr [rax+120h], 14393
	je  AVM_SystemCall_10_0_14393
	cmp word ptr [rax+120h], 15063
	je  AVM_SystemCall_10_0_15063
	cmp word ptr [rax+120h], 16299
	je  AVM_SystemCall_10_0_16299
	cmp word ptr [rax+120h], 17134
	je  AVM_SystemCall_10_0_17134
	cmp word ptr [rax+120h], 17763
	je  AVM_SystemCall_10_0_17763
	cmp word ptr [rax+120h], 18362
	je  AVM_SystemCall_10_0_18362
	cmp word ptr [rax+120h], 18363
	je  AVM_SystemCall_10_0_18363
	cmp word ptr [rax+120h], 19041
	je  AVM_SystemCall_10_0_19041
	cmp word ptr [rax+120h], 19042
	je  AVM_SystemCall_10_0_19042
	cmp word ptr [rax+120h], 19043
	je  AVM_SystemCall_10_0_19043
	cmp word ptr [rax+120h], 19044
	je  AVM_SystemCall_10_0_19044
	cmp word ptr [rax+120h], 19045
	je  AVM_SystemCall_10_0_19045
	cmp word ptr [rax+120h], 22000				  ; START OF WINDOWS 11
	je  AVM_SystemCall_10_0_22000
	cmp word ptr [rax+120h], 22621
	je  AVM_SystemCall_10_0_22621
	cmp word ptr [rax+120h], 22631
	je  AVM_SystemCall_10_0_22631
	jmp AVM_SystemCall_Unknown
AVM_SystemCall_10_0_10240:        ; Windows 10.0.10240 (1507)
	mov eax, 0018h
	jmp AVM_Epilogue
AVM_SystemCall_10_0_10586:        ; Windows 10.0.10586 (1511)
	mov eax, 0018h
	jmp AVM_Epilogue
AVM_SystemCall_10_0_14393:        ; Windows 10.0.14393 (1607)
	mov eax, 0018h
	jmp AVM_Epilogue
AVM_SystemCall_10_0_15063:        ; Windows 10.0.15063 (1703)
	mov eax, 0018h
	jmp AVM_Epilogue
AVM_SystemCall_10_0_16299:        ; Windows 10.0.16299 (1709)
	mov eax, 0018h
	jmp AVM_Epilogue
AVM_SystemCall_10_0_17134:        ; Windows 10.0.17134 (1803)
	mov eax, 0018h
	jmp AVM_Epilogue
AVM_SystemCall_10_0_17763:        ; Windows 10.0.17763 (1809)
	mov eax, 0018h
	jmp AVM_Epilogue
AVM_SystemCall_10_0_18362:        ; Windows 10.0.18362 (1903)
	mov eax, 0018h
	jmp AVM_Epilogue
AVM_SystemCall_10_0_18363:        ; Windows 10.0.18363 (1909)
	mov eax, 0018h
	jmp AVM_Epilogue
AVM_SystemCall_10_0_19041:        ; Windows 10.0.19041 (2004)
	mov eax, 0018h
	jmp AVM_Epilogue
AVM_SystemCall_10_0_19042:        ; Windows 10.0.19042 (20H2)
	mov eax, 0018h
	jmp AVM_Epilogue
AVM_SystemCall_10_0_19043:        ; Windows 10.0.19043 (21H1)
	mov eax, 0018h
	jmp AVM_Epilogue
AVM_SystemCall_10_0_19044:        ; Windows 10.0.19044 (21H2)
	mov eax, 0018h
	jmp AVM_Epilogue
AVM_SystemCall_10_0_19045:        ; Windows 10.0.19045 (22H2)
	mov eax, 0018h
	jmp AVM_Epilogue
AVM_SystemCall_10_0_22000:        ; Windows 10.0.22000 (21H2 WIN 11)
	mov eax, 0018h
	jmp AVM_Epilogue
AVM_SystemCall_10_0_22621:        ; Windows 10.0.22621 (22H2 WIN 11) 
	mov eax, 0018h
	jmp AVM_Epilogue
AVM_SystemCall_10_0_22631:        ; Windows 10.0.22631 (23H2 WIN 11)
	mov eax, 0018h
	jmp AVM_Epilogue
AVM_SystemCall_Unknown:           ; Unknown/unsupported version.
	ret
AVM_Epilogue:
	mov r10, rcx
	int 2Eh
	ret
AVM ENDP

CT PROC
	mov rax, gs:[60h]                      ; Load PEB into RAX.
CT_Check_X_X_XXXX:						   ; Check major version.
	cmp dword ptr [rax+118h], 10
	je  CT_Check_10_0_XXXX
	jmp CT_SystemCall_Unknown
CT_Check_10_0_XXXX:						   ; Check build number for Windows 10.
	cmp word ptr [rax+120h], 10240
	je  CT_SystemCall_10_0_10240
	cmp word ptr [rax+120h], 10586
	je  CT_SystemCall_10_0_10586
	cmp word ptr [rax+120h], 14393
	je  CT_SystemCall_10_0_14393
	cmp word ptr [rax+120h], 15063
	je  CT_SystemCall_10_0_15063
	cmp word ptr [rax+120h], 16299
	je  CT_SystemCall_10_0_16299
	cmp word ptr [rax+120h], 17134
	je  CT_SystemCall_10_0_17134
	cmp word ptr [rax+120h], 17763
	je  CT_SystemCall_10_0_17763
	cmp word ptr [rax+120h], 18362
	je  CT_SystemCall_10_0_18362
	cmp word ptr [rax+120h], 18363
	je  CT_SystemCall_10_0_18363
	cmp word ptr [rax+120h], 19041
	je  CT_SystemCall_10_0_19041
	cmp word ptr [rax+120h], 19042
	je  CT_SystemCall_10_0_19042
	cmp word ptr [rax+120h], 19043
	je  CT_SystemCall_10_0_19043
	cmp word ptr [rax+120h], 19044
	je  CT_SystemCall_10_0_19044
	cmp word ptr [rax+120h], 19045
	je  CT_SystemCall_10_0_19045
	cmp word ptr [rax+120h], 22000		   ; START OF WINDOWS 11
	je  CT_SystemCall_10_0_22000
	cmp word ptr [rax+120h], 22621
	je  CT_SystemCall_10_0_22621
	cmp word ptr [rax+120h], 22631
	je  CT_SystemCall_10_0_22631
	jmp CT_SystemCall_Unknown
CT_SystemCall_10_0_10240:        ; Windows 10.0.10240 (1507)
	mov eax, 00b3h
	jmp CT_Epilogue
CT_SystemCall_10_0_10586:        ; Windows 10.0.10586 (1511)
	mov eax, 00b4h
	jmp CT_Epilogue
CT_SystemCall_10_0_14393:        ; Windows 10.0.14393 (1607)
	mov eax, 00b6h
	jmp CT_Epilogue
CT_SystemCall_10_0_15063:        ; Windows 10.0.15063 (1703)
	mov eax, 00b9h
	jmp CT_Epilogue
CT_SystemCall_10_0_16299:        ; Windows 10.0.16299 (1709)
	mov eax, 00bah
	jmp CT_Epilogue
CT_SystemCall_10_0_17134:        ; Windows 10.0.17134 (1803)
	mov eax, 00bbh
	jmp CT_Epilogue
CT_SystemCall_10_0_17763:        ; Windows 10.0.17763 (1809)
	mov eax, 00bch
	jmp CT_Epilogue
CT_SystemCall_10_0_18362:        ; Windows 10.0.18362 (1903)
	mov eax, 00bdh
	jmp CT_Epilogue
CT_SystemCall_10_0_18363:        ; Windows 10.0.18363 (1909)
	mov eax, 00bdh
	jmp CT_Epilogue
CT_SystemCall_10_0_19041:        ; Windows 10.0.19041 (2004)
	mov eax, 00c1h
	jmp CT_Epilogue
CT_SystemCall_10_0_19042:        ; Windows 10.0.19042 (20H2)
	mov eax, 00c1h
	jmp CT_Epilogue
CT_SystemCall_10_0_19043:        ; Windows 10.0.19043 (21H1)
	mov eax, 00c1h
	jmp CT_Epilogue
CT_SystemCall_10_0_19044:        ; Windows 10.0.19044 (21H2)
	mov eax, 00c2h
	jmp CT_Epilogue
CT_SystemCall_10_0_19045:        ; Windows 10.0.19045 (22H2)
	mov eax, 00c2h
	jmp CT_Epilogue
CT_SystemCall_10_0_22000:        ; Windows 10.0.22000 (21H2 WIN 11)
	mov eax, 00c6h
	jmp CT_Epilogue
CT_SystemCall_10_0_22621:        ; Windows 10.0.22621 (22H2 WIN 11) 
	mov eax, 00c7h
	jmp CT_Epilogue
CT_SystemCall_10_0_22631:        ; Windows 10.0.22631 (23H2 WIN 11)
	mov eax, 00c7h
	jmp CT_Epilogue
CT_SystemCall_Unknown:           ; Unknown/unsupported version.
	ret
CT_Epilogue:
	mov r10, rcx
	int 2Eh
	ret
CT ENDP

WFSO PROC
	mov rax, gs:[60h]                           ; Load PEB into RAX.
WFSO_Check_X_X_XXXX:							; Check major version.
	cmp dword ptr [rax+118h], 10
	je  WFSO_Check_10_0_XXXX
	jmp WFSO_SystemCall_Unknown		
WFSO_Check_10_0_XXXX:							; Check build number for Windows 10.
	cmp word ptr [rax+120h], 10240
	je  WFSO_SystemCall_10_0_10240
	cmp word ptr [rax+120h], 10586
	je  WFSO_SystemCall_10_0_10586
	cmp word ptr [rax+120h], 14393
	je  WFSO_SystemCall_10_0_14393
	cmp word ptr [rax+120h], 15063
	je  WFSO_SystemCall_10_0_15063
	cmp word ptr [rax+120h], 16299
	je  WFSO_SystemCall_10_0_16299
	cmp word ptr [rax+120h], 17134
	je  WFSO_SystemCall_10_0_17134
	cmp word ptr [rax+120h], 17763
	je  WFSO_SystemCall_10_0_17763
	cmp word ptr [rax+120h], 18362
	je  WFSO_SystemCall_10_0_18362
	cmp word ptr [rax+120h], 18363
	je  WFSO_SystemCall_10_0_18363
	cmp word ptr [rax+120h], 19041
	je  WFSO_SystemCall_10_0_19041
	cmp word ptr [rax+120h], 19042
	je  WFSO_SystemCall_10_0_19042
	cmp word ptr [rax+120h], 19043
	je  WFSO_SystemCall_10_0_19043
	cmp word ptr [rax+120h], 19044
	je  WFSO_SystemCall_10_0_19044
	cmp word ptr [rax+120h], 19045
	je  WFSO_SystemCall_10_0_19045
	cmp word ptr [rax+120h], 22000				; START OF WINDOWS 11
	je  WFSO_SystemCall_10_0_22000
	cmp word ptr [rax+120h], 22621
	je  WFSO_SystemCall_10_0_22621
	cmp word ptr [rax+120h], 22631
	je  WFSO_SystemCall_10_0_22631
	jmp WFSO_SystemCall_Unknown
WFSO_SystemCall_10_0_10240:        ; Windows 10.0.10240 (1507)
	mov eax, 0004h
	jmp WFSO_Epilogue
WFSO_SystemCall_10_0_10586:        ; Windows 10.0.10586 (1511)
	mov eax, 0004h
	jmp WFSO_Epilogue
WFSO_SystemCall_10_0_14393:        ; Windows 10.0.14393 (1607)
	mov eax, 0004h
	jmp WFSO_Epilogue
WFSO_SystemCall_10_0_15063:        ; Windows 10.0.15063 (1703)
	mov eax, 0004h
	jmp WFSO_Epilogue
WFSO_SystemCall_10_0_16299:        ; Windows 10.0.16299 (1709)
	mov eax, 0004h
	jmp WFSO_Epilogue
WFSO_SystemCall_10_0_17134:        ; Windows 10.0.17134 (1803)
	mov eax, 0004h
	jmp WFSO_Epilogue
WFSO_SystemCall_10_0_17763:        ; Windows 10.0.17763 (1809)
	mov eax, 0004h
	jmp WFSO_Epilogue
WFSO_SystemCall_10_0_18362:        ; Windows 10.0.18362 (1903)
	mov eax, 0004h
	jmp WFSO_Epilogue
WFSO_SystemCall_10_0_18363:        ; Windows 10.0.18363 (1909)
	mov eax, 0004h
	jmp WFSO_Epilogue
WFSO_SystemCall_10_0_19041:        ; Windows 10.0.19041 (2004)
	mov eax, 0004h
	jmp WFSO_Epilogue
WFSO_SystemCall_10_0_19042:        ; Windows 10.0.19042 (20H2)
	mov eax, 0004h
	jmp WFSO_Epilogue
WFSO_SystemCall_10_0_19043:        ; Windows 10.0.19043 (21H1)
	mov eax, 0004h
	jmp WFSO_Epilogue
WFSO_SystemCall_10_0_19044:        ; Windows 10.0.19044 (21H2)
	mov eax, 0004h
	jmp WFSO_Epilogue
WFSO_SystemCall_10_0_19045:        ; Windows 10.0.19045 (22H2)
	mov eax, 0004h
	jmp WFSO_Epilogue
WFSO_SystemCall_10_0_22000:        ; Windows 10.0.22000 (21H2 WIN 11)
	mov eax, 0004h
	jmp WFSO_Epilogue
WFSO_SystemCall_10_0_22621:        ; Windows 10.0.22621 (22H2 WIN 11) 
	mov eax, 0004h
	jmp WFSO_Epilogue
WFSO_SystemCall_10_0_22631:        ; Windows 10.0.22631 (23H2 WIN 11)
	mov eax, 0004h
	jmp WFSO_Epilogue
WFSO_SystemCall_Unknown:           ; Unknown/unsupported version.
	ret
WFSO_Epilogue:
	mov r10, rcx
	int 2Eh
	ret
WFSO ENDP

C PROC
	mov rax, gs:[60h]					; Load PEB into RAX.
C_Check_X_X_XXXX:						; Check major version.
	cmp dword ptr [rax+118h], 10
	je  C_Check_10_0_XXXX
	jmp C_SystemCall_Unknown
C_Check_10_0_XXXX:						; Check build number for Windows 10.
	cmp word ptr [rax+120h], 10240
	je  C_SystemCall_10_0_10240
	cmp word ptr [rax+120h], 10586
	je  C_SystemCall_10_0_10586
	cmp word ptr [rax+120h], 14393
	je  C_SystemCall_10_0_14393
	cmp word ptr [rax+120h], 15063
	je  C_SystemCall_10_0_15063
	cmp word ptr [rax+120h], 16299
	je  C_SystemCall_10_0_16299
	cmp word ptr [rax+120h], 17134
	je  C_SystemCall_10_0_17134
	cmp word ptr [rax+120h], 17763
	je  C_SystemCall_10_0_17763
	cmp word ptr [rax+120h], 18362
	je  C_SystemCall_10_0_18362
	cmp word ptr [rax+120h], 18363
	je  C_SystemCall_10_0_18363
	cmp word ptr [rax+120h], 19041
	je  C_SystemCall_10_0_19041
	cmp word ptr [rax+120h], 19042
	je  C_SystemCall_10_0_19042
	cmp word ptr [rax+120h], 19043
	je  C_SystemCall_10_0_19043
	cmp word ptr [rax+120h], 19044
	je  C_SystemCall_10_0_19044
	cmp word ptr [rax+120h], 19045
	je  C_SystemCall_10_0_19045
	cmp word ptr [rax+120h], 22000		; START OF WINDOWS 11
	je  C_SystemCall_10_0_22000
	cmp word ptr [rax+120h], 22621
	je  C_SystemCall_10_0_22621
	cmp word ptr [rax+120h], 22631
	je  C_SystemCall_10_0_22631
	jmp C_SystemCall_Unknown
C_SystemCall_10_0_10240:        ; Windows 10.0.10240 (1507)
	mov eax, 000fh
	jmp C_Epilogue
C_SystemCall_10_0_10586:        ; Windows 10.0.10586 (1511)
	mov eax, 000fh
	jmp C_Epilogue
C_SystemCall_10_0_14393:        ; Windows 10.0.14393 (1607)
	mov eax, 000fh
	jmp C_Epilogue
C_SystemCall_10_0_15063:        ; Windows 10.0.15063 (1703)
	mov eax, 000fh
	jmp C_Epilogue
C_SystemCall_10_0_16299:        ; Windows 10.0.16299 (1709)
	mov eax, 000fh
	jmp C_Epilogue
C_SystemCall_10_0_17134:        ; Windows 10.0.17134 (1803)
	mov eax, 000fh
	jmp C_Epilogue
C_SystemCall_10_0_17763:        ; Windows 10.0.17763 (1809)
	mov eax, 000fh
	jmp C_Epilogue
C_SystemCall_10_0_18362:        ; Windows 10.0.18362 (1903)
	mov eax, 000fh
	jmp C_Epilogue
C_SystemCall_10_0_18363:        ; Windows 10.0.18363 (1909)
	mov eax, 000fh
	jmp C_Epilogue
C_SystemCall_10_0_19041:        ; Windows 10.0.19041 (2004)
	mov eax, 000fh
	jmp C_Epilogue
C_SystemCall_10_0_19042:        ; Windows 10.0.19042 (20H2)
	mov eax, 000fh
	jmp C_Epilogue
C_SystemCall_10_0_19043:        ; Windows 10.0.19043 (21H1)
	mov eax, 000fh
	jmp C_Epilogue
C_SystemCall_10_0_19044:        ; Windows 10.0.19044 (21H2)
	mov eax, 000fh
	jmp C_Epilogue
C_SystemCall_10_0_19045:        ; Windows 10.0.19045 (22H2)
	mov eax, 000fh
	jmp C_Epilogue
C_SystemCall_10_0_22000:        ; Windows 10.0.22000 (21H2 WIN 11)
	mov eax, 000fh
	jmp C_Epilogue
C_SystemCall_10_0_22621:        ; Windows 10.0.22621 (22H2 WIN 11) 
	mov eax, 000fh
	jmp C_Epilogue
C_SystemCall_10_0_22631:        ; Windows 10.0.22631 (23H2 WIN 11)
	mov eax, 000fh
	jmp C_Epilogue
C_SystemCall_Unknown:           ; Unknown/unsupported version.
	ret
C_Epilogue:
	mov r10, rcx
	int 2Eh
	ret
C ENDP

OP PROC
	mov rax, gs:[60h]                   ; Load PEB into RAX.
OP_Check_X_X_XXXX:						; Check major version.
	cmp dword ptr [rax+118h], 10
	je  OP_Check_10_0_XXXX
	jmp OP_SystemCall_Unknown
OP_Check_10_0_XXXX:						; Check build number for Windows 10.
	cmp word ptr [rax+120h], 10240
	je  OP_SystemCall_10_0_10240
	cmp word ptr [rax+120h], 10586
	je  OP_SystemCall_10_0_10586
	cmp word ptr [rax+120h], 14393
	je  OP_SystemCall_10_0_14393
	cmp word ptr [rax+120h], 15063
	je  OP_SystemCall_10_0_15063
	cmp word ptr [rax+120h], 16299
	je  OP_SystemCall_10_0_16299
	cmp word ptr [rax+120h], 17134
	je  OP_SystemCall_10_0_17134
	cmp word ptr [rax+120h], 17763
	je  OP_SystemCall_10_0_17763
	cmp word ptr [rax+120h], 18362
	je  OP_SystemCall_10_0_18362
	cmp word ptr [rax+120h], 18363
	je  OP_SystemCall_10_0_18363
	cmp word ptr [rax+120h], 19041
	je  OP_SystemCall_10_0_19041
	cmp word ptr [rax+120h], 19042
	je  OP_SystemCall_10_0_19042
	cmp word ptr [rax+120h], 19043
	je  OP_SystemCall_10_0_19043
	cmp word ptr [rax+120h], 19044
	je  OP_SystemCall_10_0_19044
	cmp word ptr [rax+120h], 19045
	je  OP_SystemCall_10_0_19045
	cmp word ptr [rax+120h], 22000		; START OF WINDOWS 11
	je  OP_SystemCall_10_0_22000	
	cmp word ptr [rax+120h], 22621
	je  OP_SystemCall_10_0_22621
	cmp word ptr [rax+120h], 22631
	je  OP_SystemCall_10_0_22631
	jmp OP_SystemCall_Unknown
OP_SystemCall_10_0_10240:        ; Windows 10.0.10240 (1507)
	mov eax, 0026h
	jmp OP_Epilogue
OP_SystemCall_10_0_10586:        ; Windows 10.0.10586 (1511)
	mov eax, 0026h
	jmp OP_Epilogue
OP_SystemCall_10_0_14393:        ; Windows 10.0.14393 (1607)
	mov eax, 0026h
	jmp OP_Epilogue
OP_SystemCall_10_0_15063:        ; Windows 10.0.15063 (1703)
	mov eax, 0026h
	jmp OP_Epilogue
OP_SystemCall_10_0_16299:        ; Windows 10.0.16299 (1709)
	mov eax, 0026h
	jmp OP_Epilogue
OP_SystemCall_10_0_17134:        ; Windows 10.0.17134 (1803)
	mov eax, 0026h
	jmp OP_Epilogue
OP_SystemCall_10_0_17763:        ; Windows 10.0.17763 (1809)
	mov eax, 0026h
	jmp OP_Epilogue
OP_SystemCall_10_0_18362:        ; Windows 10.0.18362 (1903)
	mov eax, 0026h
	jmp OP_Epilogue
OP_SystemCall_10_0_18363:        ; Windows 10.0.18363 (1909)
	mov eax, 0026h
	jmp OP_Epilogue
OP_SystemCall_10_0_19041:        ; Windows 10.0.19041 (2004)
	mov eax, 0026h
	jmp OP_Epilogue
OP_SystemCall_10_0_19042:        ; Windows 10.0.19042 (20H2)
	mov eax, 0026h
	jmp OP_Epilogue
OP_SystemCall_10_0_19043:        ; Windows 10.0.19043 (21H1)
	mov eax, 0026h
	jmp OP_Epilogue
OP_SystemCall_10_0_19044:        ; Windows 10.0.19044 (21H2)
	mov eax, 0026h
	jmp OP_Epilogue
OP_SystemCall_10_0_19045:        ; Windows 10.0.19045 (22H2)
	mov eax, 0026h
	jmp OP_Epilogue
OP_SystemCall_10_0_22000:        ; Windows 10.0.22000 (21H2 WIN 11)
	mov eax, 0026h
	jmp OP_Epilogue
OP_SystemCall_10_0_22621:        ; Windows 10.0.22621 (22H2 WIN 11) 
	mov eax, 0026h
	jmp OP_Epilogue
OP_SystemCall_10_0_22631:        ; Windows 10.0.22631 (23H2 WIN 11)
	mov eax, 0026h
	jmp OP_Epilogue
OP_SystemCall_Unknown:           ; Unknown/unsupported version.
	ret
OP_Epilogue:
	mov r10, rcx
	int 2Eh
	ret
OP ENDP

end
