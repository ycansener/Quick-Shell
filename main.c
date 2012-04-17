#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <ctype.h>
#define BUFFER_SIZE 1<<16
#define ARR_SIZE 1<<16
#define N 10

//Fonksiyon prototipleri
void intHandler(int);
void addToHistory(char [BUFFER_SIZE],char [N][100]);
void showHistory(int, char [N][100]);
int file_exists(const char *);
void execute_command(char *,char **);
void exec_sys_command(char *);
void catch_stdout(char *,char **,char *);
void put_stdin(char *,char **,char *);

//Kabuk yazilimi sonlandirma istegini kontrol eden degisken.
static int keepRunning = 1;

//Girilen komutlari parse eden parser.
void parse_args(char *buffer, char** args,
                size_t args_size, size_t *nargs)
{
    char *buf_args[args_size]; /* You need C99 */
    char **cp;
    char *wbuf;
    size_t i, j;

    wbuf=buffer;
    buf_args[0]=buffer;
    args[0] =buffer;

    for(cp=buf_args; (*cp=strsep(&wbuf, " \n\t")) != NULL ;)
    {
        if ((*cp != '\0') && (++cp >= &buf_args[args_size]))
            break;
    }

    for (j=i=0; buf_args[i]!=NULL; i++)
    {
        if(strlen(buf_args[i])>0)
            args[j++]=buf_args[i];
    }

    *nargs=j;
    args[j]=NULL;
}


//Girilen son komutu 10 elemanlik string dizisine yazan fonksiyon.
void addToHistory(char value[BUFFER_SIZE],char history[N][100])
{
    int i;
    for(i=9; i>-1; i--)
    {
        strcpy(history[i+1],history[i]);
    }
    strcpy(history[0],value);
}

//Belirtilen sayida gecmis komutlari gosteren fonksiyon.
void showHistory(int n, char history[N][100])
{
    int i;
    if(n>10 || n<1)
        printf("\nBellek kapasitesi asimi.(Min:1 / Max:10)\n");
    else
    {
        for(i=0; i<n; i++)
        {
            printf("%d - %s\n",i+1,history[i]);
        }
    }
}

//CTRL + C ile kernel'den gönderilen veya "exit" girilmesi durumunda el ile yarattığımız interrupt sinyalini handle eden fonksiyon.
void intHandler(int sig)
{
    char  c;
    signal(sig, SIG_IGN);
    printf("\nProgrami sonlandirmak istediginize emin misiniz ? [Y/n] ");
    c = getchar();
    if (c == 'y' || c == 'Y')
        exit(0);
    else
        signal(SIGINT, intHandler);
}

//Dosyaya yazmali ve dosyadan okumali islemler icin dosyanin varolup olmadigini kontrol eden fonksiyon.
int file_exists(const char * filename)
{
    FILE * file;
    if ( (file = fopen(filename, "r") ) != NULL )
    {
        fclose(file);
        return 1;
    }
    return 0;
}

//Tek kelimelik veya parametre alan komutlarin isletilmesini saglayan fonksiyon.
void execute_command(char *command,char **args)
{
    int pid = fork();//Cocuk surec yaratiliyor.

    if(pid == 0)//Cocuk surec
    {
        execvp(command,args);//Istenilen komutu argümanlariyla birlikte isletiyor.
        exit(EXIT_SUCCESS);//Ve sonlandiriliyor. (Boylelikle parent waitte askida kalmaktan kurtuluyor.)
    }
    else//Parent surec
    {
        waitpid(pid,NULL,0);//Cocuk surecin bitmesini bekliyor...
    }
}

//Bir dosyadaki veriler üzerinde komut calistirmayi saglayan fonksiyon.
void put_stdin(char *command,char **args,char *file_name)
{
    FILE *file;//Dosya pointeri
    int pid = fork();//Fork yapilarak child process olusturuluyor.
    if(pid == 0)//Cocuk process
    {
        file = freopen(file_name, "r", stdin);//Dosya standart inputtan gelecek verileri almak uzere aciliyor.
        execvp(command,args);//Komut isletiliyor ve stdin yerine dosyadan okunan veriler geciyor..
        fclose(file);//Dosya kapatiliyor.
        exit(EXIT_SUCCESS);//Cocuk process sonlandiriliyor..
    }
    else
    {
        waitpid(pid,NULL,0);//Parent process cocugun sonlanmasini bekliyor..
        printf("\nVeriler '%s' isimli dosyadan okundu.\n",file_name);//Ve islemin basariyla sonlandigini belirten mesaji yazdiriyor.
    }
}

//Sistem komutlarini dogrudan calistirmak icin kullanilan fonksiyon (NOT: Bu fonksiyon hic kullanilmamistir.)
void exec_sys_command(char *command)
{
    system(command);//Sistem komutlarini dogrudan kabuga yazip calistirmakla ayni islevi goruyor. Ancak bu odev icin hic kullanilmadi.
}

//Ciktisi dosyaya yonlendirilecek sistme cagrilari icin fonksiyon.
void catch_stdout(char *command,char **args,char *file_name)
{
    int pid = fork();//Child process olusturuluyor.
    FILE *file;//Dosya pointeri
    if(pid == 0)//Cocuk process
    {
        file = freopen(file_name, "w", stdout);//Dosya standart output'a yansiyan verileri alacak sekilde aciliyor.
        execvp(command,args);//Komut isletiliyor ve ciktilari stdout'tan yonlendirilerek dosyaya yazdiriliyor.
        fclose(file);//Dosya kapaniyor.
        exit(EXIT_SUCCESS);//Surec sonlandiriliyor.
    }
    else
    {
        waitpid(pid,NULL,0);//Child process'in sonlanmasi bekleniyor.
        printf("\nVeriler '%s' isimli dosyaya kaydedildi.\n",file_name);
    }
}

//popen ile pipe olusturarak, surecler arasi iletisim gerektiren durumlarda kullanilan fonksiyon.
void execute_with_pipe(char *command1)
{

    FILE *fpipe;//Pipe icin dosya pointeri tanımlaniyor.
    char line[256];

    if ( !(fpipe = (FILE*)popen(command1,"r")) )//Tanimlanan pointer uzerine bir pipe aciliyor. Pipe uzerinde
    {//Komutlar isletiliyor.
        // Pipe sifir degilse,
        perror("Problems with pipe");
        exit(1);
    }

    while ( fgets( line, sizeof line, fpipe))//Pipe'a yazilan son veriler ekrana yazdiriliyor.
    {
        printf("%s", line);
    }
    pclose(fpipe);//Pipe kapatiliyor.
}


//main fonksiyonu

int main()
{
    char buffer[BUFFER_SIZE];//stdin'den alinan verinin ilk etapta saklandigi string
    char *args[ARR_SIZE];//Parse isleminden donen argumanlarin tutuldugu dizi
    char *meanfull_command[ARR_SIZE];//Birden cok komut veya yonlendirme iceren sistem cagrilarinda
    //sistem icin anlamli argumanlarin tutuldugu dizi
    char *file_name;//Dosyali islemlerde kullanilacak olan dosya adi
    size_t nargs;//Komuutun parse edilmesi sonucu olusan argüman sayisi
    char history[N][100];//Gecmisin tutuldugu string dizisi
    char command[BUFFER_SIZE];//Bufferdan alinan komutun kopyalandigi string
    int re_execute = -1;//History uzerinden komut calistirilip calistirilmadigini kontrol eden degisken
    int parts = 0;//arguman sayisinin yedeklendigi degisken
    int i;//sayac
    int islem_turu;//Komutun parametre alan bir komut mu yoksa yonlendirmeli yada multiprocess bir islem mi oldugunu tutan degisken
    int args_state;//Belirleyici < > | argümanlarının command dizisinde kacinci eleman oldugunu tutan degisken
    int type;//yapilacak islemin < > | 'den hangisi oldugunu belirten degisken

    while(keepRunning)
    {
        signal(SIGINT,intHandler);//Interrupt sinyalinin intHandler fonksiyonu ile handle edilmesi.

        if(re_execute == -1)//Re-execute, yani history üzerinden bir komut çağrısı olmadığı sürece prompt yazilmasi.
            printf("$prompt> ");

        if(re_execute != -1 || fgets(buffer, BUFFER_SIZE, stdin) != NULL)//Bos string alindiginda segmentation fault
        {//ile karsilasilmamasi icin gerekli kontrollerin yapilmasi..
            if(re_execute != -1)//Eger re-execute islemi varsa yani historyden !N ile bir komut cagirildiysa, buna karsilik gelen degerin
            {//buffer'a kopyalanmasi ve o islemin isletilmesinin saglanmasi.
                strcpy(buffer,history[re_execute-1]);
                re_execute = -1;
            }

            strcpy(command,buffer);//Buffer ile alinan komutun yedeklenmesi.
            parse_args(buffer, args, ARR_SIZE, &nargs);//Buffer ile alinan komutun parse edilmesi

            if(args[0] != NULL)//Eger bir komut girilmis ise...
            {
                parts = nargs;//Arguman sayisinin yedeklenmesi..

                if(strcmp(args[0],"history") != 0 && command[0] != '!')//Komut history ve "!" ile baslayan bir komut degil ise
                    addToHistory(command,history);//History'e ekleniyor.

                if((strcmp(args[0],"exit") == 0 && parts == 1))//Eger komut exit ise interrupt sinyali olusturuluyor.
                {
                    raise(SIGINT);
                }
                else if(strcmp(args[0],"history") == 0)//Eger girilen komut history ise,
                {
                    if(parts == 2)
                        showHistory(atoi(args[1]),history);//Sayi belirtilmisse o sayi kadar
                    else showHistory(N,history);//Belirtilmemisse N tane gecmis komut listeleniyor.
                }
                else if(command[0] == '!' )//Eger re-execute islemi yapilacaksa
                {
                    if(atoi(&command[1]) < 1 || atoi(&command[1])>10)//Son 10 komut icerisinde bir deger girildigi kontrol ediliyor.
                        printf("Bilinmeyen komut!\n");
                    else
                        re_execute = atoi(&command[1]);//Ve istenilen komut re-execute degiskenine atanarak yukarıda ilgili komutun buffer'a atanmasi saglaniyor.
                }
                else if (parts == 1 && command[0] != '!')//Eger arguman sayisi 1 ise ve komut re-execute degil ise
                {
                    execute_command(args[0],args);//Girilen komut execute command fonksiyonuna gönderiliyor.
                }
                else if(parts > 1)//Eger birden fazla arguman iceren bir komut girilmis ise
                {
                    for(i=0; i<parts; i++)//Komut icinde ayirici karakterler araniyor.
                    {
                        if(strcmp(args[i],"<") == 0 || strcmp(args[i],">") == 0 || strcmp(args[i],"|") == 0)
                        {
                            islem_turu = 0;
                            args_state = i;//Bulunursa karakterin konumu degiskene aliniyor ve donguden cikiliyor.

                            if(strcmp(args[args_state],"<") == 0)//Bulunan karaktere gore islem tipi belirleniyor.
                                type = 0;
                            else if (strcmp(args[args_state],">") == 0)
                                type = 1;
                            else if (strcmp(args[args_state],"|") == 0)
                                type = 2;

                            break;
                        }
                        islem_turu = 1;
                    }

                    if(islem_turu == 0)//Eger ozel karakter iceren bir komut ise (>,<,| gibi)
                    {

                        if(type == 0)//Tip 0 ise sembolun < oldugu anlasiliyor. Dosyadan veri okunarak islem yapilacagi icin
                        {//oncelikle parse edilen komut sembole gore ikiye ayriliyor.
                            for(i=0; i<args_state; i++)
                            {
                                meanfull_command[i] = args[i];//Komut kismi
                            }
                            file_name = args[args_state+1];//Ve dosya adi olmak üzere...

                            if(file_exists(file_name))
                            {
                                put_stdin(meanfull_command[0],meanfull_command,file_name);//Eger belirtilen dosya dizinde var ise, put_stdin fonksiyonu ile dosyadaki veriler belirtilen komuta gore isletiliyor.
                            }
                            else printf("Dosya bulunamadi: '%s'\n",file_name);

                        }
                        else if(type == 1)//Eger type 1 ise sembolun > oldugu anlasiliyor. Isletilen komutun ciktisi
                        {//dosyaya yazilacagi icin, yine parse edilen komut sembole gore ikiye ayriliyor.
                            for(i=0; i<args_state; i++)
                            {
                                meanfull_command[i] = args[i];//Komut kismi
                            }
                            file_name = args[args_state+1];//Ve dosya adi olmak üzere...

                            catch_stdout(meanfull_command[0],meanfull_command,file_name);//Varolan bir dosya ile islem yapma zorunlulugumuz olmadigindan
                            //veriler dogrudan catch_stdout fonksiyonuna gonderiliyor.
                        }
                        else if(type == 2)//Eger type 2 ise sembolun | oldugu anlasiliyor. Surecler arasi iletisim gerektiren bir komut
                        {//oldugu icin pipe kullaniliyor.
                            execute_with_pipe(command);//Execute pipe fonksiyonu ile komut isletiliyor.
                        }
                    }
                    else if(islem_turu == 1)//Eger ozel sembol icermeyen, parametre alan bir komut girilmis ise,
                    {
                        execute_command(args[0],args);//Dogrudan execvp komutu ile fonksiyonda isletiliyor...
                    }
                }
            }
        }
        else fflush(stdin);//Eger bir komut girilmeden enter'a basilmis ise, standard input temizlenerek basa donuluyor.
    }
    return 0;//Eger interrupt sinyali yakalanmis ise, keepRunning degiskeni 0 oluyor, ve program donguden cikarak sonlaniyor.
}


