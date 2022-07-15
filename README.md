1.调用dl_iterate_phdr遍历动态链接库,获取所需动态库加载基址和位于phdr的dynamic segment信息<br>
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
2.遍历elf内存视图dynamic segment里面的section信息<br>
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
