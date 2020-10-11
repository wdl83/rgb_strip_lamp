all: sol.Makefile
	make -f sol.Makefile

clean:
	rm *.bin -f
	rm *.elf -f
	rm *.hex -f
	rm *.lst -f
	rm *.map -f
	rm *.su -f
