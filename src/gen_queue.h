#ifndef DECLARE_QUEUE_H
#define DECLARE_QUEUE_H

#define DECLARE_QUEUE(name, data_type, len_type, size)                                             \
    data_type name##_queue[size];                                                                  \
    len_type  name##_head = 0;                                                                     \
    len_type  name##_tail = 0;                                                                     \
                                                                                                   \
    boolean name##_enqueue(data_type data) {                                                       \
        len_type next_tail = (name##_tail + 1) % size;                                             \
        if (next_tail == name##_head) {                                                            \
            return false;                                                                          \
        }                                                                                          \
        name##_queue[name##_tail] = data;                                                          \
        name##_tail               = next_tail;                                                     \
        return true;                                                                               \
    }                                                                                              \
                                                                                                   \
    boolean name##_enqueue_n(data_type *data, len_type len) {                                      \
        len_type available_space = (name##_head - name##_tail - 1 + size) % size;                  \
        if (len > available_space) {                                                               \
            return false;                                                                          \
        }                                                                                          \
        for (len_type i = 0; i < len; i++) {                                                       \
            name##_queue[name##_tail] = data[i];                                                   \
            name##_tail               = (name##_tail + 1) % size;                                  \
        }                                                                                          \
        return true;                                                                               \
    }                                                                                              \
                                                                                                   \
    boolean name##_dequeue(data_type *data) {                                                      \
        if (name##_head == name##_tail) {                                                          \
            return false;                                                                          \
        }                                                                                          \
        if (data) {                                                                                \
            *data = name##_queue[name##_head];                                                     \
        }                                                                                          \
                                                                                                   \
        name##_head = (name##_head + 1) % size;                                                    \
        return true;                                                                               \
    }                                                                                              \
                                                                                                   \
    boolean name##_first(data_type *data) {                                                        \
        if (name##_head == name##_tail) {                                                          \
            return false; /* Queue is empty */                                                     \
        }                                                                                          \
        *data = name##_queue[name##_head];                                                         \
        return true;                                                                               \
    }                                                                                              \
                                                                                                   \
    boolean name##_empty() {                                                                       \
        return name##_head == name##_tail;                                                         \
    }

#endif
