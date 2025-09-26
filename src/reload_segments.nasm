global reloadSegments
reloadSegments:
   ; Reload CS register containing code selector:
   JMP   0x08:.reload_CS
.reload_CS:
   ; Reload data segment registers:
   MOV   AX, 0x10
   MOV   DS, AX
   MOV   ES, AX
   MOV   FS, AX
   MOV   GS, AX
   MOV   SS, AX
   RET
