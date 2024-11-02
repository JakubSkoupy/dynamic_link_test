#include <chrono>
#include <dlfcn.h>
#include <pthread.h>
#include <thread>
#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <sys/stat.h>
#include <sys/wait.h>
#include <assert.h>

typedef void (*func)();

void *load_lib(const char *path) {
    void *libhandle = dlopen(path, RTLD_LAZY);
    if (libhandle == NULL) { std::cerr << "Failed to load lib\n"; }
    return libhandle;
}

func load_func(void *libhandle, const char *name) {
    func fpointer = (func) dlsym(libhandle, name);
    if (fpointer == NULL) { std::cerr << "Failed to load function pointer\n"; }
    return fpointer;
}

struct LibUpdateHandler {
    pthread_t tid;
    int state = 0;
};

int make_lib() {
    int pid = fork();
    int status;
    if (pid == -1) { return -1; }

    if (pid == 0) {
        int nullFd = open("/dev/null", O_WRONLY);
        dup2(nullFd, STDOUT_FILENO);
        dup2(nullFd, STDERR_FILENO);
        close(nullFd);
        execlp("make", "make", "dlibs", NULL);
        exit(-1);
    } else {
        if (waitpid(pid, &status, 0) == -1) { return -1; }
        return 0;
    }
    assert(false);
}

void *t_check_update(void *arg) {
    auto h = (struct LibUpdateHandler *) arg;
    struct stat st;
    const char *src = "lib.cpp";
    stat(src, &st);

    struct timespec last_update = st.st_mtim;

    while (1) {
        stat(src, &st);

        if (h->state == 0 && st.st_mtim.tv_sec != last_update.tv_sec) {
            h->state = 1; //  Wait for recompilation
            make_lib();

            h->state = 2;
            last_update = st.st_mtim;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    return NULL;
}

int main() {
    // Main loop
    struct LibUpdateHandler handle;
    void *libhandle;

    func foo;

    if ((libhandle = load_lib("./lib.so")) == NULL || (foo = load_func(libhandle, "foo")) == NULL) {
        return -1;
    }

    pthread_create(&handle.tid, NULL, &t_check_update, &handle);

    while (1) {
        switch (handle.state) {
        case 0: {
            // Execute lib function
            foo();
        } break;
        case 1: {
            // Wait until recompiled
            break;
        }
        default:
            // Reload function
            dlclose(libhandle);
            if ((libhandle = load_lib("./lib.so")) == NULL ||
                    (foo = load_func(libhandle, "foo")) == NULL) {
                return -1;
            }
            handle.state = 0;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
}
