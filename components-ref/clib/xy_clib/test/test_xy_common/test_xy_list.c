#include "unity.h"
#include "xy_common.h"


void test_xy_list_init_node(void)
{
    xy_list_node *list = (xy_list_node *)0x1234; // 非NULL值

    xy_list_init_node(list);
    TEST_ASSERT_NULL(list);
}

void test_xy_list_add_note(void)
{
    xy_list_node *list = NULL;
    xy_list_node node1 = { 1, NULL };
    xy_list_node node2 = { 2, NULL };

    // 添加第一个节点
    xy_list_add_note(list, &node1);
    TEST_ASSERT_EQUAL_PTR(&node1, list);
    TEST_ASSERT_NULL(node1.next);

    // 添加第二个节点
    xy_list_add_note(list, &node2);
    TEST_ASSERT_EQUAL_PTR(&node2, list);
    TEST_ASSERT_EQUAL_PTR(&node1, node2.next);
}

void test_xy_list_add_note_tail(void)
{
    xy_list_node *list = NULL;
    xy_list_node *temp = NULL;
    xy_list_node node1 = { 1, NULL };
    xy_list_node node2 = { 2, NULL };

    // 添加第一个节点
    xy_list_add_note_tail(list, &node1, temp);
    TEST_ASSERT_EQUAL_PTR(&node1, list);
    TEST_ASSERT_NULL(node1.next);

    // 添加第二个节点
    xy_list_add_note_tail(list, &node2, temp);
    TEST_ASSERT_EQUAL_PTR(&node1, list);
    TEST_ASSERT_EQUAL_PTR(&node2, node1.next);
    TEST_ASSERT_NULL(node2.next);
}

void test_xy_list_del_node(void)
{
    xy_list_node *list = NULL;
    xy_list_node *temp = NULL;
    xy_list_node node1 = { 1, NULL };
    xy_list_node node2 = { 2, NULL };
    xy_list_node node3 = { 3, NULL };

    // 构建链表: node1 -> node2 -> node3
    xy_list_add_note_tail(list, &node1, temp);
    xy_list_add_note_tail(list, &node2, temp);
    xy_list_add_note_tail(list, &node3, temp);

    // 删除中间节点
    xy_list_del_node(list, &node2, temp);
    TEST_ASSERT_EQUAL_PTR(&node1, list);
    TEST_ASSERT_EQUAL_PTR(&node3, node1.next);
    TEST_ASSERT_NULL(node3.next);

    // 删除头节点
    xy_list_del_node(list, &node1, temp);
    TEST_ASSERT_EQUAL_PTR(&node3, list);
    TEST_ASSERT_NULL(node3.next);

    // 删除最后一个节点
    xy_list_del_node(list, &node3, temp);
    TEST_ASSERT_NULL(list);
}

void test_xy_list_for_node(void)
{
    xy_list_node *list = NULL;
    xy_list_node *temp = NULL;
    xy_list_node node1 = { 1, NULL };
    xy_list_node node2 = { 2, NULL };
    xy_list_node node3 = { 3, NULL };

    xy_list_add_note_tail(list, &node1, temp);
    xy_list_add_note_tail(list, &node2, temp);
    xy_list_add_note_tail(list, &node3, temp);

    // 测试遍历
    int count = 0;
    xy_list_node *n;
    xy_list_for_node(list, n)
    {
        count++;
    }
    TEST_ASSERT_EQUAL(3, count);
}

void test_xy_list_for_node_safe(void)
{
    xy_list_node *list = NULL;
    xy_list_node *temp = NULL;
    xy_list_node node1 = { 1, NULL };
    xy_list_node node2 = { 2, NULL };
    xy_list_node node3 = { 3, NULL };

    // 构建链表: node1 -> node2 -> node3
    xy_list_add_note_tail(list, &node1, temp);
    xy_list_add_note_tail(list, &node2, temp);
    xy_list_add_note_tail(list, &node3, temp);

    int count = 0;
    xy_list_node *n, *t;

    // 安全遍历并删除符合条件的节点
    xy_list_for_node_safe(list, n, t)
    {
        count++;
        if (n->value == 2) { // 删除值为2的节点
            xy_list_del_node(list, n, temp);
            // or break, not need to xy_list_del_node
        }
    }

    // 验证结果
    TEST_ASSERT_EQUAL(3, count); // 应该遍历所有3个节点
    TEST_ASSERT_EQUAL_PTR(&node1, list);
    TEST_ASSERT_EQUAL_PTR(&node3, node1.next); // node2应被删除
    TEST_ASSERT_NULL(node3.next);
}

int test_xy_list(void)
{
    RUN_TEST(test_xy_list_init_node);
    RUN_TEST(test_xy_list_add_note);
    RUN_TEST(test_xy_list_add_note_tail);
    RUN_TEST(test_xy_list_del_node);
    RUN_TEST(test_xy_list_for_node);
    RUN_TEST(test_xy_list_for_node_safe);
}