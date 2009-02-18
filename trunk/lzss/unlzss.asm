; LZSS decompressor - pnf

    ; input:
    ;   esi = input ptr (usually, in compressed buffer)
    ;   edi = output ptr (decompressed buffer)
    ;   ebp = end of input
    ; usage:
    ;   dl  = flags
    ;   dh  = count
    ;   ecx = length
    ;   ebx = offset

lzss_decompress:
    pushad

getflags:
    cmp     esi, ebp
    jge     done
    lodsb
    mov     dl, ah
    mov     dh, 8

decode:
    shr     dl, 1
    jc      offlen

    cmp     esi, ebp
    jge     done
    movsb
    jmp     next

offlen:
    lodsw
    ; offset
    movzx   ebx, ax
    shr     ebx, 4
    inc     ebx
    ; length
    movzx   ecx, ax
    and     ecx, 0Fh
    add     ecx, 3
    ; copy string
    mov     eax, esi
    mov     esi, edi
    sub     esi, ebx
    repne   movsb
    mov     esi, eax

next:
    dec     dh
    jnz     decode
    jmp     getflags

done:
    popad
    ret
