#ifndef _MPTREE_H
#define _MPTREE_H


#include<vector>
#include<string>
#include "sha256.h"

struct nodeflag {
	std::string hash;
	bool dirty = false;
};

struct shortnode {
	std::string Key;
	std::string Val1;		//用于叶子节点
	struct fullNode* Val2 = NULL;	//用于扩展节点
	nodeflag* flag = NULL;
};

struct fullNode {
	shortnode * children[17];
	nodeflag* flag;
	shortnode* parent;
};


struct resflag {
	int op = 0;
	std::string remain;
};

class MPTree {
public:
	MPTree():root(NULL),Findres1(NULL),Findres2(NULL),preNode(NULL){
		sha256 = new char[256];
	}

	~MPTree() {
		delete[] sha256;
	}

	//查询节点
	std::string Get(std::string key) {
		resflag* res = findMaxprefix(root,key);
		if (res->op) {
			if (res->remain.empty() && Findres2->children[16])
				return Findres2->children[16]->Val1;
			else
				return "Not Found";
		}
		else {
			if (res->remain.compare(Findres1->Key) == 0)
				return Findres1->Val1;
			else
				return "Not Found";
		}
	}

	//插入节点
	bool Insert(std::string key, std::string value) {
		//空树
		if (!root) {
			root = createShortNode(key,value,NULL);
			return true;
		}
		//先找到具有最长相同前缀的节点
		resflag* res = findMaxprefix(root, key);
		if (res->op) {	//查询到的是分支节点
			if (res->remain == "") {	//剩余搜索路径为空
				if (Findres2->children[16]) {	//节点已经创建过
					Findres2->children[16]->Key = key;
					Findres2->children[16]->Val1 = value;
					Findres2->children[16]->flag->dirty = true;
					Findres2->children[16]->flag->hash = "";
				}
				else {		//节点未创建
					Findres2->children[16] = createShortNode("",value,NULL);
				}
			}
			else{		//搜索路径不为空
				Findres2->children[decodingHP(res->remain[0])] = createShortNode(std::string(res->remain,1),value,NULL);
			}
		}
		else {		//查询到的是叶子/扩展节点
			if (res->remain.compare(Findres1->Key) == 0) {	//key完全相同
				Findres1->Val1 = value;
				Findres1->flag->dirty = true;
				Findres1->flag->hash = "";
			}
			else {	//创造分支节点
				int i;
				int N = std::min(res->remain.size(),Findres1->Key.size());
				for (i = 0;i < N;i++) {
					if (res->remain[i] != Findres1->Key[i])
						break;
				}

				fullNode* fnode = createFullNode(Findres1);
				shortnode*	snode1 = createShortNode(std::string(Findres1->Key,i),Findres1->Val1,Findres1->Val2);
				shortnode*	snode2 = createShortNode(std::string(res->remain,i),value,NULL);
				if (i == Findres1->Key.size()) {
					fnode->children[16] = snode1;
				}
				else {
					snode1->Key.erase(0,1);	//去掉开头的一个字符
					fnode->children[decodingHP(Findres1->Key[i])] = snode1;
				}

				if (i == res->remain.size()) {
					fnode->children[16] = snode2;
				}
				else {
					snode2->Key.erase(0, 1);
					fnode->children[decodingHP(res->remain[i])] = snode2;
				}
				
				Findres1->Key.erase(i);
				Findres1->Val1 = "";
				Findres1->Val2 = fnode;
				Findres1->flag->dirty = true;
				Findres1->flag->hash = "";
			}
		}
		return true;
	}

	//删除节点
	bool Delete(std::string key) {
		preNode = NULL;
		resflag* res = findMaxprefix(root,key);
		if (res->op) {
			if (res->remain.empty() && Findres2->children[16]){
				delete(Findres2->children[16]->flag);
				delete(Findres2->children[16]);
				Findres2->children[16] = NULL;
				Findres2->flag->dirty = true;
				Findres2->flag->hash = "";
				mergeNode(Findres2);
				return true;
			}
			else
				return false;
		}
		else {
			if (res->remain.compare(Findres1->Key) == 0) {
				delete(Findres1->flag);
				if (preNode) {
					for (int i = 0;i < 16;++i) {
						if (preNode->children[i] == Findres1) {
							delete(Findres1);
							preNode->children[i] = NULL;
							preNode->flag->dirty = true;
							preNode->flag->hash = "";
						}
					}
					mergeNode(preNode);
				}
				return true;
			}
			else
				return false;
		}
	}

	//UPDATE
	bool Update(std::string key,std::string value) {
		if (value.empty())
			return Delete(key);
		else
			return Insert(key,value);
	}

	//COMMIT :: 返回root的hash值
	std::string Commit() {	
		commitDfs(root);
		return root->flag->hash;
	}

	void Print() {
		printfDfs(root,0);
		return;
	}

private:
	shortnode* root;
	shortnode* Findres1;	//用于查找
	fullNode* Findres2;		//用于查找
	fullNode* preNode;		//删除中的前置节点
	char* sha256;

	void printfDfs(shortnode* root, size_t depth) {
		if (!root->Val1.empty())
			std::cout << std::string(2*depth, '-') + "叶子节点:Key(\"" + root->Key + "\") + Val(" + root->Val1 + ")" << std::endl;
		else
		{
			std::cout << std::string(2*depth, '-') + "扩展节点:Key(\"" + root->Key + "\")" << std::endl;
			printfDfs(root->Val2,depth+1);
		}
	}

	void printfDfs(fullNode* root,size_t depth) {
		if(root->children[16])
			std::cout << std::string(2*depth, '-') + "分支节点:Key(\"\") + Val(" + root->children[16]->Val1 + ")" << std::endl;
		else
			std::cout << std::string(2*depth, '-') + "分支节点:(null)" << std::endl;
		for (int i = 0;i < 16;++i) {
			if(root->children[i])
				printfDfs(root->children[i],depth+1);
		}
	}

	bool commitDfs(shortnode* root) {
		if (!root->Val1.empty()) {		//如果是叶子节点
			if (!root->flag->dirty) {
				//std::cout << root->Key+ " " + root->flag->hash<< std::endl;
				return false;	
			}
			else
			{
				std::string tmp = root->Key + root->Val1;
				StrSHA256(tmp.c_str(),tmp.size(),sha256);
				root->flag->dirty = false;
				root->flag->hash = std::string(sha256);
				//std::cout << root->Key + " " + root->flag->hash << std::endl;
				return true;
			}
		}
		else {		//扩展节点
			if (!commitDfs(root->Val2) && !root->flag->dirty) {
				//std::cout << root->Key + " " + root->flag->hash << std::endl;
				return false;			//表示没有修改
			}
			else
			{
				std::string tmp = root->Key + root->Val2->flag->hash;	//当前节点的key以及对应引用节点的hash
				StrSHA256(tmp.c_str(), tmp.size(), sha256);
				root->flag->dirty = false;
				root->flag->hash = std::string(sha256);
				//std::cout << root->Key + " " + root->flag->hash << std::endl;
				return true;			//表示已经修改过
			}
		}
	}

	bool commitDfs(fullNode* root) {
		bool op = root->flag->dirty;
		for (int i = 0;i < 17;i++) {
			if(root->children[i])
				op = commitDfs(root->children[i]) || op;
		}

		if (!op) {
			//std::cout << "fullnode "+ root->flag->hash << std::endl;
			return false;
		}
		else {
			std::string tmp;
			for (int i = 0;i < 17;i++) {
				if (root->children[i])
					tmp += root->children[i]->flag->hash;
			}
			StrSHA256(tmp.c_str(), tmp.size(), sha256);
			root->flag->dirty = false;
			root->flag->hash = std::string(sha256);
			//std::cout << "fullnode " + root->flag->hash << std::endl;
			return true;
		}
	}

	//寻找最长公共前缀的节点
	resflag* findMaxprefix(shortnode* head, std::string key) {
		int X = head->Key.size();
		int Y = key.size();
		Findres1 = head;
		if (X >= Y) {
			if (head->Key.compare(key) == 0 && head->Val2) {
				Findres2 = head->Val2;
				return getresflag(1, "");
			}
			return getresflag(0, key);
		}
		else {
			if (key.find(head->Key) != std::string::npos) {
				if (head->Val2) {
					if (head->Val2->children[decodingHP(key[X])]) {
						preNode = head->Val2;
						return findMaxprefix(head->Val2->children[decodingHP(key[X])], std::string(key, X+1));
					}
					else {
						Findres2 = head->Val2;
						return getresflag(1, std::string(key, X));
					}
				}
				else
					return getresflag(0, key);
			}
			else {
				return getresflag(0, key);
			}
		}
	}

	shortnode* createShortNode(std::string key,std::string val1, fullNode* val2) {
		shortnode* ret = new shortnode();
		ret->Key = key;
		ret->Val1 = val1;
		ret->Val2 = val2;
		ret->flag = new nodeflag();
		ret->flag->hash = "";
		ret->flag->dirty = true;
		return ret;
	}

	fullNode* createFullNode(shortnode* parent) {
		fullNode* ret = new fullNode;
		memset(ret->children,0,17*sizeof(shortnode*));
		ret->flag = new nodeflag();
		ret->flag->hash = "";
		ret->flag->dirty = true;
		ret->parent = parent;
		return ret;
	}

	resflag* getresflag(int op,std::string remain) {
		resflag* ret = new resflag();
		ret->op = op;
		ret->remain = remain;
		return ret;
	}

	//Delete后如果分支节点的孩子只剩一个，需要合并
	void mergeNode(fullNode* fnode) {
		int count=0;
		int N = -1;
		for (int i = 0;i <= 16;i++) {
			if (fnode->children[i]) {
				count++;
				N = i;
			}
			if (count > 1)
				return;
		}
		std::string newKey = fnode->parent->Key + encodingHP(N) + fnode->children[N]->Key;
		std::string newVal = fnode->children[N]->Val1;

		fnode->parent->Key = newKey;
		fnode->parent->Val1 = newVal;
		fnode->parent->Val2 = NULL;
		fnode->parent->flag->dirty = true;
		fnode->parent->flag->hash = "";

		delete(fnode->children[N]->flag);
		delete(fnode->children[N]);
		delete(fnode->flag);
		delete(fnode);
		return;
	}

	int decodingHP(char ch) {
		if (ch <= '9')
			return ch - '0';
		else
			return ch - 'a' + 10;
	}

	char encodingHP(int i) {
		if (i <= 9)
			return i + '0';
		else
			return i - 10 + 'a';
	}

};

#endif // !_MPTREE_H