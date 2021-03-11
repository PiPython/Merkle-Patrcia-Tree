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
	std::string Val1;		//����Ҷ�ӽڵ�
	struct fullNode* Val2 = NULL;	//������չ�ڵ�
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

	//��ѯ�ڵ�
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

	//����ڵ�
	bool Insert(std::string key, std::string value) {
		//����
		if (!root) {
			root = createShortNode(key,value,NULL);
			return true;
		}
		//���ҵ��������ͬǰ׺�Ľڵ�
		resflag* res = findMaxprefix(root, key);
		if (res->op) {	//��ѯ�����Ƿ�֧�ڵ�
			if (res->remain == "") {	//ʣ������·��Ϊ��
				if (Findres2->children[16]) {	//�ڵ��Ѿ�������
					Findres2->children[16]->Key = key;
					Findres2->children[16]->Val1 = value;
					Findres2->children[16]->flag->dirty = true;
					Findres2->children[16]->flag->hash = "";
				}
				else {		//�ڵ�δ����
					Findres2->children[16] = createShortNode("",value,NULL);
				}
			}
			else{		//����·����Ϊ��
				Findres2->children[decodingHP(res->remain[0])] = createShortNode(std::string(res->remain,1),value,NULL);
			}
		}
		else {		//��ѯ������Ҷ��/��չ�ڵ�
			if (res->remain.compare(Findres1->Key) == 0) {	//key��ȫ��ͬ
				Findres1->Val1 = value;
				Findres1->flag->dirty = true;
				Findres1->flag->hash = "";
			}
			else {	//�����֧�ڵ�
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
					snode1->Key.erase(0,1);	//ȥ����ͷ��һ���ַ�
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

	//ɾ���ڵ�
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

	//COMMIT :: ����root��hashֵ
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
	shortnode* Findres1;	//���ڲ���
	fullNode* Findres2;		//���ڲ���
	fullNode* preNode;		//ɾ���е�ǰ�ýڵ�
	char* sha256;

	void printfDfs(shortnode* root, size_t depth) {
		if (!root->Val1.empty())
			std::cout << std::string(2*depth, '-') + "Ҷ�ӽڵ�:Key(\"" + root->Key + "\") + Val(" + root->Val1 + ")" << std::endl;
		else
		{
			std::cout << std::string(2*depth, '-') + "��չ�ڵ�:Key(\"" + root->Key + "\")" << std::endl;
			printfDfs(root->Val2,depth+1);
		}
	}

	void printfDfs(fullNode* root,size_t depth) {
		if(root->children[16])
			std::cout << std::string(2*depth, '-') + "��֧�ڵ�:Key(\"\") + Val(" + root->children[16]->Val1 + ")" << std::endl;
		else
			std::cout << std::string(2*depth, '-') + "��֧�ڵ�:(null)" << std::endl;
		for (int i = 0;i < 16;++i) {
			if(root->children[i])
				printfDfs(root->children[i],depth+1);
		}
	}

	bool commitDfs(shortnode* root) {
		if (!root->Val1.empty()) {		//�����Ҷ�ӽڵ�
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
		else {		//��չ�ڵ�
			if (!commitDfs(root->Val2) && !root->flag->dirty) {
				//std::cout << root->Key + " " + root->flag->hash << std::endl;
				return false;			//��ʾû���޸�
			}
			else
			{
				std::string tmp = root->Key + root->Val2->flag->hash;	//��ǰ�ڵ��key�Լ���Ӧ���ýڵ��hash
				StrSHA256(tmp.c_str(), tmp.size(), sha256);
				root->flag->dirty = false;
				root->flag->hash = std::string(sha256);
				//std::cout << root->Key + " " + root->flag->hash << std::endl;
				return true;			//��ʾ�Ѿ��޸Ĺ�
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

	//Ѱ�������ǰ׺�Ľڵ�
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

	//Delete�������֧�ڵ�ĺ���ֻʣһ������Ҫ�ϲ�
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