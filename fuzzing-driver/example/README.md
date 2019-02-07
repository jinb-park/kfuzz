# Fuzzing the example driver with syzkaller

## Set up syzkaller

- For basic set up on syzkaller, see [this article](https://github.com/google/syzkaller/blob/master/docs/linux/setup_ubuntu-host_qemu-vm_x86-64-kernel.md).

- But, There is a different settings to be needed.
  - Install go 1.9.5 (above article recommends you to install go 1.8.1, do not follow that guide)
  - Install below packages before build go.
  ```
  sudo apt-get install gcc-aarch64-linux-gnu gcc-arm-linux-gnueabi gcc-powerpc64le-linux-gnu clang-format
  ```
  - Other settings are exactly same to above article.

## Put the buggy module poc_lkm.c into Linux kernel

- Instructions are as follows.
```
cp -f poc_lkm.c /path/to/your/linux/lib
cd /path/to/your/linux/lib
vim Makefile
(++) obj-y += poc_lkm.o  // adding this line to Makefile
make CC=/your/gcc  // build linux kernel again 
```

## Apply syscall description of poc_lkm.c to syzkaller

- poc_lkm.txt is syscall description that is applied to syzkaller.

```
cp -f poc_lkm.txt /your/syzkaller/sys/linux/
cd /your/syzkaller
make bin/syz-extract
./bin/syz-extract -os linux -arch amd64 -sourcedir /path/to/your/linux poc_lkm.txt
make generate
make  // build syzkaller
```

## Run syzkaller to fuzz poc_lkm.c

- poc_lkm.cfg is a syzkaller configuration to only fuzz poc_lkm.c.

```
cp -f poc_lkm.cfg /your/syzkaller/
cd /your/syzkaller
./bin/syz-manager --config=poc_lkm.cfg
```

- If you re-run syzkaller with initial state, remove all files inside workdir, run syzkaller again.
```
cd /your/syzkaller/workdir
rm -rf *
```

## Details

- To get to know how this example works, See [this blog](https://jinb-park.blogspot.com/2019/02/how-syzkaller-works-03-fuzzing-driver.html).
