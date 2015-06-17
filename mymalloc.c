#include <stdlib.h>
#include <string.h>

struct list_t {
    struct list_t *next;
};

typedef struct list_t list_t;

static list_t *root = NULL;
static list_t *end = NULL;

static inline void* get_ptr(list_t *list)
{
    return &((char*)list)[sizeof(list_t)];
}

void *my_malloc(size_t size)
{
    list_t *p = (list_t*)malloc(sizeof(list_t) + size);
    p->next = NULL;
    
    if(root == NULL) {
        root = p;
        end = root;
    } else {
        end->next = p;
        end = p;
    }
    
    return get_ptr(p);
}

static list_t* list_search(void *ptr, list_t **parent)
{
    list_t *curr, *next;
    curr = NULL;
    next = root;
    while(next != NULL) {
        if( get_ptr(next) == ptr ) {
            *parent = curr;
            return next;
        }
        curr = next;
        next = curr->next;
    }
    return NULL;
}

int my_free(const void *ptr)
{
    list_t *parent, *curr;
    curr = list_search((void*)ptr, &parent);
    if(curr == NULL) {
        return 1;
    }
    
    if(parent == NULL) {
        root = curr->next;
    } else {
        parent->next = curr->next;
    }
    
    if(curr->next == NULL) {
        end = parent;
    }
    free(curr);
    return 0;
}

void my_free_all()
{
    list_t *curr, *next;
    curr = root;
    while(curr) {
        next = curr->next;
        free(curr);
        curr = next;
    }
}
