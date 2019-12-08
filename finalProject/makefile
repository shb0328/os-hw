# file		makefile
# date		2008/11/12
# author	kkamagui 
# brief		OS �̹����� �����ϱ� ���� make ����

# �⺻������ ���带 ������ ���
all: BootLoader Kernel32 Kernel64 Utility Disk.img

# ��Ʈ �δ� ���带 ���� ��Ʈ �δ� ���͸����� make ����
BootLoader:
	@echo 
	@echo ============== Build Boot Loader ===============
	@echo 
	
	make -C 00.BootLoader

	@echo 
	@echo =============== Build Complete ===============
	@echo 
	
# ��ȣ ��� Ŀ�� �̹����� �����ϱ� ���� ��ȣ ��� ���͸����� make ����
Kernel32:
	@echo 
	@echo ============== Build 32bit Kernel ===============
	@echo 
	
	make -C 01.Kernel32

	@echo 
	@echo =============== Build Complete ===============
	@echo 

# IA-32e ��� Ŀ�� �̹����� �����ϱ� ���� IA-32e ��� ���͸����� make ����
Kernel64:
	@echo 
	@echo ============== Build 64bit Kernel ===============
	@echo 
	
	make -C 02.Kernel64

	@echo 
	@echo =============== Build Complete ===============
	@echo 

	
# OS �̹��� ����
Disk.img: 00.BootLoader/BootLoader.bin 01.Kernel32/Kernel32.bin 02.Kernel64/Kernel64.bin
	@echo 
	@echo =========== Disk Image Build Start ===========
	@echo 

	./ImageMaker $^

	@echo 
	@echo ============= All Build Complete =============
	@echo 
	
# ��ƿ��Ƽ ����
Utility:
	@echo 
	@echo =========== Utility Build Start ===========
	@echo 

	make -C 04.Utility

	@echo 
	@echo =========== Utility Build Complete ===========
	@echo 
	
	
# �ҽ� ������ ������ ������ ���� ����	
clean:
	make -C 00.BootLoader clean
	make -C 01.Kernel32 clean
	make -C 02.Kernel64 clean
	make -C 04.Utility clean
	rm -f Disk.img

run:
	qemu-system-x86_64 -L . -fda Disk.img -m 64 -localtime -M pc -rtc base=localtime -monitor stdio -hda HDD.img

hdd:
	qemu-img create HDD.img 20M
