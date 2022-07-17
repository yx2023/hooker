void mytestcall()
{
    printf("hello mytestcall\n");
}
void main()
{
    hook("testLib", "testcall", mytestcall);
    testFunc();
}
