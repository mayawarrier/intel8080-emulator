;
; Source for bin/INTERRUPT.COM
; Halt until an interrupt is received, then exit.
;

bdos  equ 05h          ;basic DOS
      org 100h
      jmp start

msg1  db 'Waiting for interrupt...',0dh,0ah,'$'
msg2  db 'Received! Exit to DOS.',0dh,0ah,'$'

start ei
      mvi c,9          ;set function 9 (print string)
      lxi d,msg1
      call bdos
      hlt              ;wait for an interrupt
done  lxi d,msg2
      call bdos
      rst 0            ;exit