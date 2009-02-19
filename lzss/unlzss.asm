; LZSS decompressor - pnf

    ; input:
    ;   esi = input ptr (usually, in compressed buffer)
    ;   edi = output ptr (decompressed buffer)
    ;   ebp = end of input
    ; usage:
    ;   dl  = lzss flags
    ;   dh  = lzss count
    ;   ecx = length
    ;   ebx = tmp

lzss_decompress:
    pushad
    xor     eax, eax

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
    ; length
    mov     ecx, eax
    and     ecx, 0Fh
    add     ecx, 3
    ; offset
    mov     ebx, esi ;also try with push/pop esi
    shr     eax, 4
    not     eax
    lea     esi, [edi+eax]
    ; copy string
    repne   movsb
    mov     esi, ebx

next:
    dec     dh
    jnz     decode
    jmp     getflags

done:
    popad
    ret
