sudo losetup -P /dev/loop10 ./b.img
sudo ./format /dev/loop10p5 
sudo losetup -d /dev/loop10