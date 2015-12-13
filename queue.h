#ifndef QUEUE_H_
#define QUEUE_H_
#define NULL (void*)0

#define Q_HEAD(name, type)                                          \
struct name {                                                       \
        struct type *first;                                         \
        struct type **last;                                         \
}

#define Q_ENTRY(type)                                               \
struct {                                                            \
        struct type *next;                                          \
        struct type **prev;                                         \
}

#define Q_INIT(head) do {                                           \
        (head)->first = NULL;                                       \
        (head)->last = &(head)->first;                              \
} while (0)

#define Q_INSERT_HEAD(head, elm, field) do {                        \
        if (((elm)->field.next = (head)->first) != NULL)            \
                (head)->first->field.prev =                         \
                    &(elm)->field.next;                             \
        else                                                        \
                (head)->last = &(elm)->field.next;                  \
        (head)->first = (elm);                                      \
        (elm)->field.prev = &(head)->first;                         \
} while (0)

#define Q_INSERT_TAIL(head, elm, field) do {                        \
        (elm)->field.next = NULL;                                   \
        (elm)->field.prev = (head)->last;                           \
        *(head)->last = (elm);                                      \
        (head)->last = &(elm)->field.next;                          \
} while (0)

#define Q_INSERT_AFTER(head, listelm, elm, field) do {              \
        if (((elm)->field.next = (listelm)->field.next) != NULL)    \
                (elm)->field.next->field.prev =                     \
                    &(elm)->field.next;                             \
        else                                                        \
                (head)->last = &(elm)->field.next;                  \
        (listelm)->field.next = (elm);                              \
        (elm)->field.prev = &(listelm)->field.next;                 \
} while (0)

#define Q_INSERT_BEFORE(listelm, elm, field) do {                   \
        (elm)->field.prev = (listelm)->field.prev;                  \
        (elm)->field.next = (listelm);                              \
        *(listelm)->field.prev = (elm);                             \
        (listelm)->field.prev = &(elm)->field.next;                 \
} while (0)

#define Q_REMOVE(head, elm, field) do {                             \
        if (((elm)->field.next) != NULL)                            \
                (elm)->field.next->field.prev =                     \
                    (elm)->field.prev;                              \
        else                                                        \
                (head)->last = (elm)->field.prev;                   \
        *(elm)->field.prev = (elm)->field.next;                     \
} while (0)

#define Q_FOREACH(var, head, field)                                 \
        for ((var) = ((head)->first); (var); (var) = ((var)->field.next))

#define Q_EMPTY(head)               ((head)->first == NULL)
#define Q_FIRST(head)               ((head)->first)
#define Q_NEXT(elm, field)          ((elm)->field.next)

#define Q_LAST(head, headname)                                      \
        (*(((struct headname *)((head)->last))->last))
#define Q_PREV(elm, headname, field)                                \
        (*(((struct headname *)((elm)->field.prev))->last))

#endif
