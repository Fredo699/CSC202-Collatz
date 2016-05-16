#include "btree.h"
#define NULL ((void *) 0)
treeNode *bt_insert(treeNode *node,unsigned long data)
{
        if(node==NULL)
        {
                treeNode temp;
                temp.data = data;
                temp.left = temp.right = NULL;
                return &temp;
        }

        if(data >(node->data))
        {
                node->right = bt_insert(node->right,data);
        }
        else if(data < (node->data))
        {
                node->left = bt_insert(node->left,data);
        }
        /* Else there is nothing to do as the data is already in the tree. */
        return node;

}

// Return a zero if not found, non-zero if found.
int bt_find(treeNode *node, unsigned long data)
{
        if(node==NULL)
        {
                /* Element is not found */
                return 0;
        }
        if(data > node->data)
        {
                /* Search in the right sub tree. */
                return bt_find(node->right,data);
        }
        else if(data < node->data)
        {
                /* Search in the left sub tree. */
                return bt_find(node->left,data);
        }
        else
        {
                /* Element Found */
                return 1;
        }
}