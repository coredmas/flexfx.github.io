xcc -report -O3 -lquadflash flexfx.xn flexfx.a %1.c -o %1.xe
xflash --no-compression --factory-version 14.3 --upgrade 1 %1.xe -o %1.bin
