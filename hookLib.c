#define _GNU_SOURCE
#include <link.h>
#include <string.h>
#include <elf.h>
#include <sys/mman.h>
struct hookData
{
    char* soName;
    char* symName;
    ElfW(Addr) soAddr;
};

int hookCallback(struct dl_phdr_info *info, size_t size, void *data)
{
    printf("%s\n", info->dlpi_name);
    ElfW(Addr) baseAddr = 0;
    ElfW(Addr) virtAddr = 0;
    int segmentsNum = 0;
    if (strstr(info->dlpi_name, ((struct hookData*)data)->soName) != NULL) {
        printf("find the obj addr is 0x%016lx\n", info->dlpi_addr);
        baseAddr = info->dlpi_addr;

        for(ElfW(Half) i = 0; i < info->dlpi_phnum; i++) {
            if (info->dlpi_phdr[i].p_type == PT_DYNAMIC) {
                virtAddr = baseAddr + info->dlpi_phdr[i].p_vaddr;
                printf("the dynamic segment virt addr is 0x%016lx\n", virtAddr);
                segmentsNum = info->dlpi_phdr[i].p_filesz / sizeof(Elf64_Dyn);
                break;
            }
        }
        if (virtAddr == 0) {
            printf("Invalid Addr\n");
        }
        Elf64_Dyn *dynSegments = (Elf64_Dyn*)virtAddr;
        Elf64_Sym* symTable = 0;
        void* strTable = 0;
        Elf64_Rela* relaTable = 0;
        Elf64_Rela* jmpRelTable = 0;
        Elf64_Xword relTableSize = 0;
        Elf64_Xword relTableEntrySize = 0;

        Elf64_Xword jumpRelTableSize = 0;
        Elf64_Xword jumpRelTableEntrySize = sizeof(Elf64_Rela);
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
        if (!relaTable) {
            printf("PANIC : No relaTable\n");
        }
        printf("index is %d , char is %c\n", 117, ((char*)strTable+118));
        
        for (int i = 0; i < jumpRelTableSize/jumpRelTableEntrySize; i++) {
            
            unsigned long symIndex = ELF64_R_SYM(jmpRelTable[i].r_info);
            unsigned long symStrOffset = symTable[symIndex].st_name;
            char* symName = (char*)(strTable + symStrOffset);
            printf("symName is : %s\n", symName);

           
            if (strcmp(symName, ((struct hookData*)data)->symName) == 0) {
                printf("find the sym : %s\n", symName);
                printf("r_offset is 0x%016lx\n", jmpRelTable[i].r_offset);
                
                unsigned long page_size = getpagesize();
                unsigned long page_start = (Elf64_Addr)(jmpRelTable[i].r_offset + baseAddr) & (~(page_size -1));
                mprotect((void*)page_start, page_size, PROT_READ|PROT_WRITE|PROT_EXEC);
                printf("soAddr is 0x%016lx\n", ((struct hookData*)data)->soAddr);
                *(void**)(jmpRelTable[i].r_offset + baseAddr) = ((struct hookData*)data)->soAddr;
                printf("soAddr is 0x%016lx\n", ((struct hookData*)data)->soAddr);
                //break;
            }
        }
        
    }
    return 0;
}
int hook(const char* soName, const char* symbolName, void* funcAddr)
{
    int ret = 0;
    struct hookData data = {.soName=soName, .symName=symbolName, .soAddr=funcAddr};
    if ((ret = dl_iterate_phdr(hookCallback, (void*)&data)) != 0) {
        printf("walkthrough failure\n");
        return -1;
    }
}