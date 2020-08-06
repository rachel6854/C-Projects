#include <sys/types.h>/*for pthread_t*/
#include <stdlib.h>/*for malloc*/
#include <pthread.h>/*for init*/
#include <dirent.h>/*for opendir*/
#include <sys/stat.h>/*for struct stat*/
#include <fnmatch.h>/*for fnmatch*/
#include <signal.h> /*for sigaction*/
#include <stdio.h> /*for printf*/
#include <string.h> /*for strcmp,strchr*/
#define MAX_THREADS 100


pthread_t threads[MAX_THREADS];
int num_of_threads;
int counter = 0;
int threads_counter = 0;
size_t error_threads = 0;
char* search_str;
pthread_mutex_t  lock;
pthread_cond_t not_empty;

struct node
{
    char *path;
    struct node *next;

};

typedef struct node node;

node *q_head;
node *q_tail;



void release_queue()
{
    while (q_head)
    {
        node *tmp = q_head -> next;
        free (q_head -> path);
        free (q_head);
        q_head = tmp;
    }
}

void cancel_handler(int signal)
{
    for (size_t i = 0; i < num_of_threads; ++i)
    {
        if (pthread_self() != threads[i])
            pthread_cancel(threads[i]);

    }

    pthread_mutex_destroy(&lock);

    printf("Done searching, found %d files\n", counter);
    if(search_str)
    {
        free(search_str);
        search_str=NULL;
    }

    exit(0);
}

void sigint_handler(int signal)
{
    for (size_t i = 0; i < num_of_threads; ++i)
    {
        pthread_cancel(threads[i]);
    }
    release_queue();

    pthread_mutex_destroy(&lock);
    if(search_str)
    {
        free(search_str);
        search_str=NULL;
    }

    printf("Search stopped, found %d files\n", counter);

    exit(0);
}

char* dequeue()
{
    pthread_mutex_lock( &lock );

    while (!q_head)
        pthread_cond_wait(&not_empty ,&lock);
    ++threads_counter;
    node *tmp = q_head;
    q_head = q_head -> next;
    pthread_mutex_unlock( &lock );
    char* directory = tmp -> path;
    if(tmp)
    {
        free(tmp);
        tmp=NULL;
    }

    return directory;
}

void enqueue(char *folder)
{
    pthread_mutex_lock( &lock );
    node * new_node=malloc(sizeof(node));
    new_node->path=malloc(strlen(folder)+1);
    strcpy(new_node->path,folder);
    new_node->next=NULL;
    if (!q_head)
    {
        q_head = new_node;
        q_tail = new_node;
    }
    else
    {
        q_tail->next = new_node;
        q_tail = q_tail->next;
    }
    pthread_cond_signal(&not_empty);
    pthread_mutex_unlock( &lock );
}

void treat_file(const char* path)
{

    if (fnmatch(search_str ,strrchr(path ,'/') + 1 ,0) == 0)
    {
        printf("%s\n", path);
        __sync_fetch_and_add(&counter, 1);
    }
}

void exit_browse_well()
{
    perror("Error in opening directory");
    __sync_fetch_and_sub(&error_threads, 1);
    if (error_threads == num_of_threads)
    {
        release_queue();
        pthread_mutex_destroy(&lock);
        if(search_str)
        {
            free(search_str);
            search_str=NULL;
        }

        printf("Done searching, found %d files\n", counter);


        exit(1);
    }
    pthread_exit(NULL);
}

void* browse()
{

    while (1)
    {
        char *path = dequeue();
        DIR *dir = opendir(path);
        if (dir == NULL)
        {
        exit_browse_well();
        }

        struct dirent *entry;
        struct stat dir_stat;
        while((entry = readdir(dir)) != NULL)
        {
            char buff[strlen(path)+strlen(entry -> d_name) + 2];
            sprintf(buff,"%s/%s", path, entry->d_name);
            stat(buff,&dir_stat);

            if(strcmp(entry->d_name,"..") != 0 && strcmp(entry->d_name, ".") != 0)
            {
                if((dir_stat.st_mode & __S_IFMT) == __S_IFDIR)
                    enqueue(buff);
                else
                    treat_file(buff);
            }
        }
        if(path)
        {
            free(path);
            path=NULL;
        }


        closedir(dir);
        __sync_fetch_and_sub(&threads_counter, 1);
        if (!q_head && !threads_counter)
        {
            raise (SIGUSR1);
        }
    }

}


int main(int argc, char **argv)
{
    struct sigaction cancel_sa;
    cancel_sa.sa_handler = cancel_handler;
    sigaction(SIGUSR1, &cancel_sa, NULL);
    struct sigaction sigint_sa;
    sigint_sa.sa_handler = sigint_handler;
    sigaction(SIGINT, &sigint_sa, NULL);

    if(argc != 4)
    {
        perror("Missing or extra arguments. mfind requieres exactly three arguments\n") ;
        printf("%d",argc);
        exit(1);
    }
    search_str = malloc(strlen(argv[2] + 3));
    sprintf(search_str, "%c%s%c", '*', argv[2], '*');
    num_of_threads = atoi(argv[3]);
    enqueue(argv[1]);
    q_tail = q_head;
    int rc = pthread_mutex_init( &lock, NULL );
    if( rc )
    {
        printf("ERROR in pthread_mutex_init(): "
               "%s\n", strerror(rc));
        exit(-1);
    }
    for (size_t i = 0; i < num_of_threads; ++i)
    {
        int rc = pthread_create(&threads[i], NULL, browse, (void *)i);
        if (rc)
        {
            printf("ERROR in pthread_create(): %s\n", strerror(rc));
            exit(-1);
        }
    }
    for (size_t i = 0; i < num_of_threads; ++i)
        pthread_join(threads[i], NULL);
    if(search_str)
    {
        free(search_str);
        search_str=NULL;
    }

    exit(0);
}