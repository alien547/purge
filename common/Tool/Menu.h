#ifndef MENU_H
#define MENU_H

#include <memory>
#include <unordered_map>
#include <vector>
#include <stack>
#include <functional>
#include "ParticleSystem.h"

struct TreeNode{
    TreeNode(const std::string& val):data(val), parent(nullptr){}
    std::string data;
    TreeNode* parent;
    std::vector<std::unique_ptr<TreeNode>> children;
    std::function<void()> on_enter;
};

//目录及初始界面
class Menu{
    public:
        ParticleSystem particle_system;

        std::stack<int> path;//记录目录路径
        std::unique_ptr<TreeNode> root;//目录根节点
        std::unordered_map<std::string, TreeNode*> TN_node;
        TreeNode* now_node;
        bool can_exit=true;

        std::function<void()> on_start_page, on_end_page;

        void wait_key_press();
        TreeNode* add_child(TreeNode* parent, const std::string& child_data);
        void change_state(bool& state);
        void choose();
        void start_page();
        void end_page();
};

#endif

