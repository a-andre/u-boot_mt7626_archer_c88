#基本概念
##uboot
uboot由CPU的ROM引导启动，一般的逻辑是：
firststage：
初始化底层硬件如DRAM，建立栈等C语言处理环境，用汇编写
secondstage：
初始化硬件，解压实际的uboot代码，用C写
thirdstage：
真正的uboot初始化，高级功能如命令解析，webserver等等，引导操作系统内核如linux或vxworks。
secondstage和thirdstage没有太大差异，纯粹是因为uboot大部分代码被压缩，多了一个解压的过程。

uboot并不是必须的；如vxworks支持单镜像引导方式，相当于vxworks本身集成了firststage secondstage thirdstage等。
但这样会造成调试不便（比如vxworks镜像有问题导致引导失败），需要有一个恢复环境，因此现在的软件都带有uboot。

##双uboot
所谓的双uboot，指的是样机本身有一个uboot，不会被升级修改；升级软件也带有一个uboot。
第一个uboot被称为factory uboot，第二个被称为normal uboot.
factory uboot一般带有webserver，用于引导软件失败时进入恢复模式。
事实上并不需要normal uboot；由于第一个uboot不会被升级，如果需要修改一些开机启动配置，可以在vxworks或linux内核的初始化中修改。
但由于厂商一般都会把底层DRAM等的初始化放在uboot，不好移植到vxworks或linux内核，所以都习惯于在升级软件再带一个uboot；如果uboot要修改，再改这个uboot。

此外，由于两个uboot都有类似的初始化操作，需要特别注意某些硬件重复初始化可能有问题。

##恢复模式
第一个uboot带有恢复模式，如果开机引导失败，会进入恢复模式；该模式有简单的页面可供恢复固件。

###恢复页面
1. 是否需要校验签名？
视情况而定，uboot是开源的，而校验签名需要uboot带有公钥。虽然公钥可以公开，但此处不确定是否有安全问题。
不校验签名的另一个目的是某些地区可能有openwrt的升级需求。
2. 
##SPL
由于新硬件的初始化越来越复杂，在uboot新的代码中，firststage secondstage被SPL代替。

#代码说明
这是当前mt7626方案使用的uboot源码；理论上mt7622和mt7623也支持，但不能确定。

这个uboot是mtk在较新的uboot源码上修改的。
mt7626在代码中的芯片代号是leopard.

##配置文件
每个机型均只有config配置文件，如factory.config和normal.config，分别对应工厂uboot和升级uboot。
不要引进其他配置文件。每个uboot对应一个配置文件。

双uboot通过CONFIG_FACTORY_UBOOT和CONFIG_NORMAL_UBOOT以及其他参数区分开。

##编译步骤
1. 将机型的uboot配置复制到uboot目录的.config

2. 执行
```
make menuconfig
```
将会生成代码使用的多个配置文件如autoconf.h和autoconf.mk

执行
```
./make_config.sh
```
建立一些必要的软链接和释放mtk提供的.o文件。windows下不支持软链接，所以代码库中不包含这些文件。

3. 编译
```
make CROSS_COMPILE_PATH=/home/ubuntu/toolchian/usr/bin
```
如果有双uboot，需要重复执行以上步骤。每个uboot有且仅有一个config文件，不要把依赖于config生成的配置文件如autoconf.h放到机型配置下。

目前也可以直接执行makeUboot.sh完成编译操作，但步骤1依然需要。