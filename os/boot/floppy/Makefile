# added by mingxuan 2019-5-17
os/boot/floppy/boot.bin : os/boot/floppy/boot.asm os/boot/floppy/include/load.inc os/boot/floppy/include/fat12hdr.inc
	$(ASM) $(ASMBFLAGS) -o $@ $<

# added by mingxuan 2019-5-17
os/boot/floppy/loader.bin : os/boot/floppy/loader.asm os/boot/floppy/include/load.inc os/boot/floppy/include/fat12hdr.inc os/boot/floppy/include/pm.inc
	$(ASM) $(ASMBFLAGS) -o $@ $<
