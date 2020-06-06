//
// Created by zheyu on 6/4/20.
//

#ifndef HTTP_SERVER_UTILITY_H
#define HTTP_SERVER_UTILITY_H
#define LOG_DIR "../logs/log"
class Utility {
public:
    /**
     * @def add a node to the tail of the queue
     * @param node: the node which needs to be added into the queue
     * @param nodes: the tail dummy node of a list
     */
    template <class T>
    static void add_node(T *node, T *nodes) {
        T *temp = nodes->prev;
        nodes->prev = node;
        node->next = nodes;
        node->prev = temp;
        temp->next = node;
    }

    /**
     * @def remove and return node from the head of the queue
     * @tparam T
     * @param head: the head dummy node of a list
     * @param tail: the tail dummy node of a list
     * @return: the removed node
     */
    template<class T>
    static T* remove_node(T *head, T *tail) {
        if(head->next == tail) {
            return nullptr;
        }
        T *result = head->next;
        head->next = result->next;
        head->next->prev = head;
        return result;
    }
};
#endif //HTTP_SERVER_UTILITY_H
