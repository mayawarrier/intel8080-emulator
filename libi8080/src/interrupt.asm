;
; Source for bin/INTERRUPT.COM
; Waits for A to become non-zero then exits.
;

bdos  equ 05h
      org 100h
      jmp start

msg1  db 'Waiting for interrupt...',0dh,0ah,'$'
msg2  db 'Received! Quit to CP/M.',0dh,0ah,'$'

start ei
      mvi c,9          ;set BDOS function 9 (print string)
      xra a            ;clear A
      lxi d,msg1
      call bdos
loop  jnz done
      jmp loop
done  lxi d,msg2
      call bdos
      rst 0            ;exit to WBOOT