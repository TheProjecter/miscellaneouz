.586
.model flat, stdcall
option casemap :none


  assume fs:nothing

  .data

  .code

start:

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
    cld

getflags:
    cmp     esi, ebp
    jge     done
    lodsb
    mov     dl, al
    mov     dh, 8

decode:
    shr     dl, 1
    jc      offlen

    cmp     esi, ebp
    jge     done
    movsb
    jmp     next

offlen:
    xor     eax, eax
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
    rep     movsb
    mov     esi, ebx

next:
    dec     dh
    jnz     decode
    jmp     getflags

done:
    popad
    ret

end start