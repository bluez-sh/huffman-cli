#include <bits/stdc++.h>
#include <fstream>
using namespace std;

struct Node {
	int freq;
	unsigned char ch;
	struct Node *left;
	struct Node *right;
};

Node* create_node() {
	Node *newNode = new Node;
	newNode->freq = 0;
	newNode->ch = '~';
	newNode->left = nullptr;
	newNode->right = nullptr;

	return newNode;
}

struct compare {
	bool operator()(Node* n1, Node* n2) {
		return (n1->freq > n2->freq);
	}
};

Node* huffman(const vector<unsigned char> &chars, const vector<int> &freq, int n) {
	priority_queue< Node*, vector<Node*>, compare > min_heap; 	

	for(int i = 0; i < n; i++) {
		Node *newNode = create_node();
		newNode->freq = freq[i];
		newNode->ch = chars[i];

		min_heap.push(newNode);
	}

	Node *z, *x, *y;
	while(min_heap.size() != 1) {
		z = create_node();

		x = min_heap.top();
		min_heap.pop();
		y = min_heap.top();
		min_heap.pop();

		z->left = x;
		z->right = y;
		z->freq = x->freq + y->freq;
		min_heap.push(z);
	}

	return min_heap.top();
}

map<unsigned char, string> make_table(Node* node, string str, map<unsigned char, string> &table) {
	if(node == nullptr)
		return table;

	if(node->left == nullptr && node->right == nullptr) {
		table.insert(pair<unsigned char, string>(node->ch, str));
		return table;
	}

	make_table(node->left, str+"0", table);
	make_table(node->right, str+"1", table);	
	return table;
}

void write_header(fstream &out, const map<unsigned char, string> &table) {
	short tab_size = table.size();	
	out.write((char*)&tab_size, sizeof(short));
	
	unsigned char ch;
	short str_size;
	string str;
	for(auto i: table) {
		ch = i.first;
		str_size = i.second.size();
		str = i.second;

		out.write((char*)&ch, sizeof(unsigned char));
		out.write((char*)&str_size, sizeof(short));
		out.write(str.c_str(), str_size);
	}	
}

map<unsigned char, string> read_header(fstream &in) {
	map<unsigned char, string> table;
	short tab_size;
	in.read((char*)&tab_size, sizeof(short));

	unsigned char ch;
	short str_size;
	string str;
	while(tab_size--) {
		in.read((char*)&ch, sizeof(unsigned char));
		in.read((char*)&str_size, sizeof(short));	
		
		char *cstr = new char[str_size+1];
		in.read(cstr, str_size);
		
		cstr[str_size] = '\0';
		str = cstr;

		table.insert(pair<unsigned char, string>(ch, str));	
		delete [] cstr;
	}

	return table;
}


fstream open_file(string fname, bool read, bool bin, long *fsize = nullptr) {
	fstream f;

	if(read) {
		if(bin)
			f.open(fname, ios::in | ios::binary);
		else
			f.open(fname, ios::in);
	} else {
		if(bin)
			f.open(fname, ios::out | ios::binary);
		else
			f.open(fname, ios::out);
	}

	if(!f) {
		cout << "Error opening file!" << endl;
		exit(1);
	}

	if(read) {
		f.seekg(0, ios::end);		
		*fsize = f.tellg();
		f.seekg(0, ios::beg);
	}

	return f;
}


void decompress_file(const char *file_name) {
	string fileNameStr(file_name);
	long size;
	fstream in = open_file(fileNameStr, true, true, &size);

	map<unsigned char, string> table;
	table = read_header(in);

	long tableBytes = in.tellg();

	//for(auto i: table) {
		//cout << i.first << " " << i.second << endl;
	//}

	Node *root = create_node();
	string str;
	for(auto i: table) {
		str = i.second;	

		Node *cur = root;
		for(unsigned i = 0; i < str.size(); i++) {
			if(str[i] == '0') {
				if(cur->left == nullptr) {
					Node *newNode = create_node();
					cur->left = newNode;
				}	
				cur = cur->left;
			} else {
				if(cur->right == nullptr) {
					Node *newNode = create_node();
					cur->right = newNode;
				}
				cur = cur->right;
			}
		}	

		cur->ch = i.first;
	}

	char* fileBuff = new char[size-tableBytes];	
	long m = 0;
	in.read(fileBuff, size-tableBytes);
	in.close();

	char* uzbuff = new char[100000000];
	int k = 0;

	fstream out = open_file(fileNameStr.substr(0, fileNameStr.size()-2), false, false);
	unsigned char chunk;
	bool bit;
	Node *cur = root;
	for(long i = 0; i < size-tableBytes; i++) {
		chunk = fileBuff[m++];
		for(int i = 7; i >= 0; i--) {
			bit = (chunk & (1 << i))? true: false;		
			if(bit)
				cur = cur->right;
			else 
				cur = cur->left;

			if(cur->left == nullptr && cur->right == nullptr) {
				uzbuff[k++] = cur->ch;	
				cur = root;
			}	
		}	
	}

	out.write(uzbuff, k);
	out.close();

	delete [] uzbuff;
	delete [] fileBuff;
}


void compress_file(const char *file_name) {
	string fileNameStr(file_name);	
	long size;
	fstream in = open_file(fileNameStr, true, true, &size);

	char *fileBuff = new char[size];
	in.read(fileBuff, size);
	in.close();
 
	int count[256] = {0};	
	vector<unsigned char> chars;
	vector<int> freq;

	for(int i = 0; i < size; i++) 
		count[(unsigned int)fileBuff[i] % 256]++;

	for(int i = 0; i < 256; i++) {
		if(count[i] != 0) {
			chars.push_back(i);
			freq.push_back(count[i]);
		}
	}

	Node* root = huffman(chars, freq, chars.size());
	
	map<unsigned char, string> table;
	table = make_table(root, "", table);

	//for(auto i: table) {
		//cout << i.first << " " << i.second << endl;
	//}

	char *zbuff = new char[100000000];
	string code;
	int k = 0;
	unsigned char chunk = 0;
	int chunkSize = 8;
	for(int i = 0; i < size; i++) {
		 code = table.find(fileBuff[i])->second;
		 for(unsigned j = 0; j < code.size(); j++) {
			 if(chunkSize == 0) {
				 zbuff[k++] = chunk;
				 chunkSize = 8;
				 chunk = 0;
			 }
			 int bit = code[j]-'0';
			 chunk |= (bit << --chunkSize);
		 }
	}


	fstream out = open_file(fileNameStr+".z", false, true);
	write_header(out, table);
	out.write(zbuff, k);
	out.close();

	delete [] fileBuff;
	delete [] zbuff;
}


int main(int argc, char** argv) {
	if(argc == 2) {
		string opt(argv[1]);
		if(opt == "-h" || opt == "--help") {
			cout << "Syntax: ./huffman {option} <input_file>" << endl;
			cout << "\nOptions:" << endl;
			cout << "\tcompress\t\tCompress the input file." << endl;
			cout << "\tinflate \t\tDecompress the input file." << endl;	
			cout << "\t-h	   \t\tDisplay this help menu." << endl;
			exit(0);
		}
	}
	if(argc < 3) {
		cout << "Invalid syntax!" << endl;
		cout << "Try -h option for help." << endl;
		exit(1);
	}

	string opt(argv[1]);
	if(opt == "compress") {
		compress_file(argv[2]);	
		cout << "File compressed successfully!" << endl;
	}		
	else if(opt == "inflate") {	
		decompress_file(argv[2]);
		cout << "File decompressed successfully!" << endl;
	}
	else {
		cout << "Invalid option \"" << opt << "\"!"<< endl;
		cout << "Try -h option for help." << endl;
	} 

	return 0;
}
