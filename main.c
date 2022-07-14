#define NULL (void*)0
typedef int(*func)();
extern int replaceFunc();
int say()
{
    printf("replace replaceFunc\n");
}
int main()
{
    /* directly replace the dynamic func addr in got */
#if 0
    void** p = (void**)0x404038;
    *p = (void*)say;

    replaceFunc();
#endif


    hook("dynLib","puts",say);
    printf("hook done\n");
    replaceFunc();
}