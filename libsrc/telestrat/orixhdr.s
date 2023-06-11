;
; By Debrune Jérôme <jede@oric.org>

;

    ; The following symbol is used by the linker config. file
    ; to force this module to be included into the output file.
    .export __ORIXHDR__:abs = 1

    ; These symbols, also, come from the configuration file.
    .import __MAIN_START__, __MAIN_LAST__

; ------------------------------------------------------------------------
; Orix header

.segment        "ORIXHDR"

    .byte   $01, $00                ; Non C64 marker (same as o65 format)

    .byte   "ori"                   ; Magic number

    .byte   $01                     ; Version of the header
    .byte   $00                     ; Cpu type. 6502: 0
    .byte   $00
    .byte   $00, $00                ; Type of language
    .byte   $00, $00                ; OS version

    .byte   $00                     ; Reserved
    .byte   $00                     ; Auto or not

    .word   __MAIN_START__          ; Address of start of file
    .word   __MAIN_LAST__ - 1       ; Address of end of file
    .word   __MAIN_START__          ; Address of start of file

