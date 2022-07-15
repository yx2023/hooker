<a>https://man7.org/linux/man-pages/man5/elf.5.html</a><br>
<a>https://man7.org/linux/man-pages/man3/dl_iterate_phdr.3.html</a><br>
### Program header (Phdr)<br>
Phdr是elf在内存视图里的segments，segment是文件视图里section的集合，dynamic segment里面的信息可以用来定位其他section<br>
![image](https://user-images.githubusercontent.com/109275975/179177259-fc9cc322-23bb-4681-ba84-6b90349273b0.png)

### Section header (Shdr)<br>
Shdr是文件视图里面section信息集合<br>

![image](https://user-images.githubusercontent.com/109275975/179179952-156223e9-0be7-495c-8f6c-d38ca553881e.png)


### 文件视图和内存视图的映射关系
.dynSym <-> SYMTAB<br>
.dynStr <-> STRTAB<br>
.rela.plt <-> JMPREL<br>
.got <-> PLTGOT<br>
.rela.dyn <-> RELA<br>
![image](https://user-images.githubusercontent.com/109275975/179184783-3bccf62d-a682-4ce7-9abd-aabcc0215cf5.png)

### PLT和GOT
简单来说plt是一段跳转到存放函数入口地址的got表的代码
![image](https://user-images.githubusercontent.com/109275975/179194527-1232433d-8e73-4c55-a4d2-196b5c7247b6.png)

<a>https://lzz5235.github.io/2015/12/08/pltgot.html</a>

### 1.调用dl_iterate_phdr遍历动态链接库,获取所需动态库加载基址和位于phdr的dynamic segment信息<br>
```
int dl_iterate_phdr(
                 int (*callback)(struct dl_phdr_info *info,
                                 size_t size, void *data),
                 void *data);
```
dlpi_addr: 动态库基址<br>
dlpi_phdr: segments结构体<br>
```
struct dl_phdr_info {
               ElfW(Addr)        dlpi_addr;  /* Base address of object */
               const char       *dlpi_name;  /* (Null-terminated) name of
                                                object */
               const ElfW(Phdr) *dlpi_phdr;  /* Pointer to array of
                                                ELF program headers
                                                for this object */
               ElfW(Half)        dlpi_phnum; /* # of items in dlpi_phdr */

               /* The following fields were added in glibc 2.4, after the first
                  version of this structure was available.  Check the size
                  argument passed to the dl_iterate_phdr callback to determine
                  whether or not each later member is available.  */

               unsigned long long dlpi_adds;
                               /* Incremented when a new object may
                                  have been added */
               unsigned long long dlpi_subs;
                               /* Incremented when an object may
                                  have been removed */
               size_t dlpi_tls_modid;
                               /* If there is a PT_TLS segment, its module
                                  ID as used in TLS relocations, else zero */
               void  *dlpi_tls_data;
                               /* The address of the calling thread's instance
                                  of this module's PT_TLS segment, if it has
                                  one and it has been allocated in the calling
                                  thread, otherwise a null pointer */
           };

```
#### 代码
```
    int hookCallback(struct dl_phdr_info *info, size_t size, void *data)
    {
      printf("%s\n", info->dlpi_name);
      ElfW(Addr) baseAddr = 0;
      ElfW(Addr) virtAddr = 0;
      int segmentsNum = 0;
      if (strstr(info->dlpi_name, ((struct hookData*)data)->soName) != NULL) {
        printf("find the obj addr is 0x%016lx\n", info->dlpi_addr);
        baseAddr = info->dlpi_addr;
    }
```
### 2.在phdr里面搜索dynmic segment，里面有各个section的信息
```
typedef struct {
               Elf32_Word  p_type;    /* Segment type */
               Elf32_Off   p_offset;  /* Segment file offset */
               Elf32_Addr  p_vaddr;   /* Segment virtual address */
               Elf32_Addr  p_paddr;   /* Segment physical address */
               Elf32_Word  p_filesz;  /* Segment size in file */
               Elf32_Word  p_memsz;   /* Segment size in memory */
               Elf32_Word  p_flags;   /* Segment flags */
               Elf32_Word  p_align;   /* Segment alignment */
           } Elf32_Phdr;
```
#### 代码
```
        for(ElfW(Half) i = 0; i < info->dlpi_phnum; i++) {
            if (info->dlpi_phdr[i].p_type == PT_DYNAMIC) {
                virtAddr = baseAddr + info->dlpi_phdr[i].p_vaddr;
                printf("the dynamic segment virt addr is 0x%016lx\n", virtAddr);
                segmentsNum = info->dlpi_phdr[i].p_filesz / sizeof(Elf64_Dyn);
                break;
            }
        }
```
### 3.遍历elf内存视图dynamic segment里面的section信息<br>
<br>
DT_RElA:    &emsp;&emsp;对应文件视图里的rela.dyn，存储的是外部引用的全局变量符号信息<br>
DT_JMPREL:  &emsp;&emsp;对应的是文件视图里rela.plt，是外部引用的函数符号信息<br>
<br>

表内元素：<br>
```
          typedef struct {
              Elf64_Addr r_offset;      //入口地址位于动态库中的相对地址
              uint64_t   r_info;        //symTab内的索引，需要ELF64_R_SYM(r_info)转化
              int64_t    r_addend;
          } Elf64_Rela;
```  
DT_STRTAB:  &emsp;elf中字符串的表，符号名等<br>
DT_SYMTAB:  &emsp;符号表<br>
表内元素：<br>
```
           typedef struct {
               uint32_t      st_name;   //该符号的字符串命位于STRTAB的起始位置索引
               unsigned char st_info;
               unsigned char st_other;
               uint16_t      st_shndx;
               Elf64_Addr    st_value;
               uint64_t      st_size;
           }Elf64_Sym;
```
#### 代码
```
      for (int i = 0; i < segmentsNum; i++) {
            switch(dynSegments[i].d_tag) {
                case DT_PLTRELSZ:
                    printf("DT_PLTRELSZ is %d\n", dynSegments[i].d_un.d_val);
                    jumpRelTableSize = dynSegments[i].d_un.d_val;
                    break;
                case DT_PLTGOT:
                    printf("DT_PLTGOT Addr is 0x%016lx\n", dynSegments[i].d_un.d_ptr);
                    break;
                case DT_SYMTAB:
                    symTable = (Elf64_Sym*)(dynSegments[i].d_un.d_ptr);
                    printf("DT_SYMTAB Addr is 0x%016lx\n", dynSegments[i].d_un.d_ptr);
                    break;
                case DT_SYMENT:
                    printf("DT_SYMENT size if %d\n", dynSegments[i].d_un.d_val);
                    break;
                case DT_RELASZ:
                    relTableSize = dynSegments[i].d_un.d_val;
                    break;
                case DT_RELAENT:
                    relTableEntrySize = dynSegments[i].d_un.d_val;
                    break;
                case DT_RELA:
                    relaTable = (Elf64_Rela*)(dynSegments[i].d_un.d_ptr);
                    printf("DT_RELA Addr is 0x%016lx\n", dynSegments[i].d_un.d_ptr);
                    break;
                case DT_JMPREL:
                    jmpRelTable = (Elf64_Rela*)(dynSegments[i].d_un.d_ptr);
                    printf("DT_JMPREL Addr is 0x%016lx\n", dynSegments[i].d_un.d_ptr);
                    break;
                case DT_STRTAB:
                    strTable = (void*)(dynSegments[i].d_un.d_ptr);
                    printf("DT_STRTAB Addr is 0x%016lx\n", dynSegments[i].d_un.d_ptr);
                    break;
                default:
                    break;
            }
        }
```
### 4.根据dynmic中获取的各个section, <br>
&emsp;1）遍历jmpRel，r_info表示每个外部函数符号在symTab中的索引<br>
&emsp;2）获取symTab中该函数符号的st_name，表示在strTab中的字符串位置<br>
&emsp;3）比对是不是想替换的函数符号<br>
&emsp;4）根据jmpRel中该符号的r_offset + baseAddr找到.got表中该函数地址<br>
#### 代码
```
    unsigned long symIndex = ELF64_R_SYM(jmpRelTable[i].r_info);
            unsigned long symStrOffset = symTable[symIndex].st_name;
            char* symName = (char*)(strTable + symStrOffset);
            printf("symName is : %s\n", symName);

           
            if (strcmp(symName, ((struct hookData*)data)->symName) == 0) {
                printf("find the sym : %s\n", symName);
                printf("r_offset is 0x%016lx\n", jmpRelTable[i].r_offset);
```
### 5. 用mprotect对r_offset + baseAddr所在的页进行权限设置并替换函数入口
#### 代码
```
      unsigned long page_size = getpagesize();
                unsigned long page_start = (Elf64_Addr)(jmpRelTable[i].r_offset + baseAddr) & (~(page_size -1));
                mprotect((void*)page_start, page_size, PROT_READ|PROT_WRITE|PROT_EXEC);
                printf("soAddr is 0x%016lx\n", ((struct hookData*)data)->soAddr);
                *(void**)(jmpRelTable[i].r_offset + baseAddr) = ((struct hookData*)data)->soAddr;
                printf("soAddr is 0x%016lx\n", ((struct hookData*)data)->soAddr);
```
