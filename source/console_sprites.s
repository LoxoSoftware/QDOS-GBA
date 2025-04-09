
@{{BLOCK(cnsspr)

@=======================================================================
@
@	cnsspr, 16x8@4, 
@	+ palette 2 entries, not compressed
@	+ 2 tiles not compressed
@	Total size: 4 + 64 = 68
@
@	Time-stamp: 2025-04-01, 18:44:04
@	Exported by Cearn's GBA Image Transmogrifier, v0.8.3
@	( http://www.coranac.com/projects/#grit )
@
@=======================================================================

	.section .rodata
	.align	2
	.global cnssprTiles		@ 64 unsigned chars
cnssprTiles:
	.hword 0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000
	.hword 0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000
	.hword 0xFFFF,0x00FF,0xFFFF,0x00FF,0xFFFF,0x00FF,0xFFFF,0x00FF
	.hword 0xFFFF,0x00FF,0xFFFF,0x00FF,0xFFFF,0x00FF,0xFFFF,0x00FF

	.section .rodata
	.align	2
	.global cnssprPal		@ 4 unsigned chars
cnssprPal:
	.hword 0x0000,0x0010

@}}BLOCK(cnsspr)
