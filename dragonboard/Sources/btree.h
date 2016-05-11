#ifndef BTREE_H
#define BTREE_H
typedef struct treeNode
{
        unsigned long data;
        struct treeNode *left;
        struct treeNode *right;

}treeNode;

treeNode *bt_insert(treeNode *node, unsigned long data);
int bt_find(treeNode *node, unsigned long data);

#endif