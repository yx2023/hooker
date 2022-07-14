遍历elf内存视图dynamic segment里面的section信息<br>
<br>
DT_RElA:    对应文件视图里的rela.dyn，存储的是外部引用的全局变量符号信息<br>
DT_JMPREL:  对应的是文件视图里rela.plt，是外部引用的函数符号信息<br>
<br>
表内元素：<br>
typedef struct {<br>
               Elf64_Addr r_offset;      //入口地址位于动态库中的相对地址<br>
               uint64_t   r_info;        //symTab内的索引，需要ELF64_R_SYM(r_info)转化<br>
               int64_t    r_addend;<br>
           } Elf64_Rela;<br>
           
DT_STRTAB:  elf中字符串的表，符号名等<br>
DT_SYMTAB:  符号表<br>
表内元素：<br>
typedef struct {<br>
               uint32_t      st_name;   //该符号的字符串命位于STRTAB的起始位置索引<br>
               unsigned char st_info;<br>
               unsigned char st_other;<br>
               uint16_t      st_shndx;<br>
               Elf64_Addr    st_value;<br>
               uint64_t      st_size;<br>
           } Elf64_Sym;<br>
<br>

![image](https://user-images.githubusercontent.com/109275975/178944160-8f4a12b2-f47e-4de4-b9a9-08721af333a8.png)
