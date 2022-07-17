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
        baseAddr = info->dlpi_addr;

        for(ElfW(Half) i = 0; i < info->dlpi_phnum; i++) {
            if (info->dlpi_phdr[i].p_type == PT_DYNAMIC) {
                virtAddr = baseAddr + info->dlpi_phdr[i].p_vaddr;
                segmentsNum = info->dlpi_phdr[i].p_filesz / sizeof(Elf64_Dyn);
                break;
            }
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
                    jumpRelTableSize = dynSegments[i].d_un.d_val;
                    break;
                case DT_PLTGOT:
                    break;
                case DT_SYMTAB:
                    symTable = (Elf64_Sym*)(dynSegments[i].d_un.d_ptr);
                    break;
                case DT_SYMENT:
                    break;
                case DT_RELASZ:
                    relTableSize = dynSegments[i].d_un.d_val;
                    break;
                case DT_RELAENT:
                    relTableEntrySize = dynSegments[i].d_un.d_val;
                    break;
                case DT_RELA:
                    relaTable = (Elf64_Rela*)(dynSegments[i].d_un.d_ptr);
                    break;
                case DT_JMPREL:
                    jmpRelTable = (Elf64_Rela*)(dynSegments[i].d_un.d_ptr);
                    break;
                case DT_STRTAB:
                    strTable = (void*)(dynSegments[i].d_un.d_ptr);
                    break;
                default:
                    break;
            }
        }
        if (!relaTable) {
            printf("PANIC : No relaTable\n");
        }
        
        for (int i = 0; i < jumpRelTableSize/jumpRelTableEntrySize; i++) {
            
            unsigned long symIndex = ELF64_R_SYM(jmpRelTable[i].r_info);
            unsigned long symStrOffset = symTable[symIndex].st_name;
            char* symName = (char*)(strTable + symStrOffset);

           
            if (strcmp(symName, ((struct hookData*)data)->symName) == 0) {               
                unsigned long page_size = getpagesize();
                unsigned long page_start = (Elf64_Addr)(jmpRelTable[i].r_offset + baseAddr) & (~(page_size -1));
                mprotect((void*)page_start, page_size, PROT_READ|PROT_WRITE|PROT_EXEC);
                *(void**)(jmpRelTable[i].r_offset + baseAddr) = ((struct hookData*)data)->soAddr;
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