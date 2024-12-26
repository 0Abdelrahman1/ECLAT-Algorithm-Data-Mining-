#include <iostream>
#include <fstream>
#include <vector>
#include <set>
#include <map>
#include <string>
#include <algorithm>
#include <bitset>
#include <iomanip>


using namespace std;

class StringSplitter
{
private:
    static bool isValid(char c)
    {
        return c != ',' && c != '\"';
    }

public:
    static vector<string> split(string s)
    {
        /*
                a function that just splits each line
        */

        vector<string> ret;
        string tmp = "";

        for (int i = 0; i < s.size(); i++)
        {
            if (!isValid(s[i]) && tmp != "")
                ret.push_back(tmp), tmp = "";
            else if (isValid(s[i]))
                tmp += s[i];
        }
        if (tmp != "") ret.push_back(tmp);

        return ret;
    }
};

class FileReader
{
private:
    friend class FileIterator;

    //  item    transactions
    map<string, set<string>> data;
    fstream file;

    FileReader(string fileName)
    {
        //opens file
        file.open(fileName, ios::in);
        if (!file.is_open())
        {
            cout << "\n\n\nNOT A VALID FILE NAME.\nPlease Retry.\n\n\n";
            system("pause");
            exit(0);
        }

        //read the first line to determine if the file is horizontal or vertical
        string currLine;
        getline(file, currLine);
        vector<string> firstLine = StringSplitter::split(currLine);
        for (int i = 0; i < firstLine[0].size(); i++) firstLine[0][i] = toupper(firstLine[0][i]);

        //go process the rest of the lines
        if (firstLine[0] == "TID")
            readLines(0, 1);
        else
            readLines(1, 0);

        //don't forget to close the file
        file.close();
    }

    void readLines(int isVertical, int isHorizontal)
    {
        //if the flag was 0 it will remain 0 but if 1 it will become a mask of ones
        isVertical *= -1, isHorizontal *= -1;

        string currLine;
        while (getline(file, currLine))
        {
            vector<string> tmp = StringSplitter::split(currLine);
            for (int i = 1; i < tmp.size(); i++)
            {
                /*
                * usage of bitwise and here make it possible to insert items
                *
                * consider this vector (Vertical Fromat) -> {I1, T1, T2}
                * here you want to do -> tmp[ v[0] ].push_back( v[1] ), tmp[0].push_back( v[2] )
                *
                * but here in this vector (Horizontal Fromat) -> {T1, I1, I2}
                * here you want to do -> tmp[ v[1] ].push_back( v[0] ), tmp[2].push_back( v[0] )
                *
                */

                data[tmp[(i & isHorizontal)]].insert(tmp[(i & isVertical)]);
            }
        }
    }
};

class FileIterator
{
private:
    map<string, set<string>>::iterator it;
    int currIteratorState;
    FileReader* fileReader;

public:
    FileIterator(string fileName)
    {
        fileReader = new FileReader(fileName);
        it = fileReader->data.begin();
    }

    bool hasNext()
    {
        if (currIteratorState == 3)
        {
            //moves the iterator when the user got the item and the transactions
            currIteratorState = 0;
            it = next(it);
        }

        return it != fileReader->data.end();
    }

    string getCurrItem()
    {
        currIteratorState |= 1;
        return it->first;
    }

    vector<string> getCurrItemTransactions()
    {
        currIteratorState |= 2;
        return vector<string>(it->second.begin(), it->second.end());
    }
};

struct Rule
{
    vector<string> left = {};
    vector<string> right = {};
};

template <class T>
using Data = map<vector<T>, map<T, vector<T>>>;

int minSup;
long double minConf;
vector<string> trans_hash, items_hash;

template <class T>
vector<T> operator+ (vector<T> v, T s)
{
    v.push_back(s);
    return v;
}

template <class T>
vector<T> operator+ (vector<T> v, vector<T>& t)
{
    for (auto& i : t)
        v.push_back(i);
    return v;
}

template <class T>
ostream& operator<< (ostream& ostream1, const vector<T>& v)
{
    for (auto& i : v)
        ostream1 << i << " ";
    return ostream1;
}

vector<int> intersection_merge(vector<int> a, vector<int> b)
{
    vector<int> nw;
    for (int i = 0, j = 0; i < a.size() && j < b.size(); )
        if (a[i] == b[j])
            nw.push_back(a[i]), i++, j++;
        else if (a[i] < b[j])
            i++;
        else
            j++;
    return nw;
}

void CtoL(Data<int>& mp)
{
    for (auto it = mp.begin(); it != mp.end(); )
    {
        auto& prefix = it->first;
        auto& table = it->second;
        for (auto jt = table.begin(); jt != table.end(); )
            if (jt->second.size() < minSup)
                jt = table.erase(jt);
            else
                jt++;

        if (table.empty())
            it = mp.erase(it);
        else
            it++;
    }
}

Data<int> next_C_table(Data<int>& mp)
{
    Data<int> nmp;
    for (auto& i : mp)
    {
        auto prefix = i.first;
        auto table = i.second;

        for (auto it = table.begin(); it != table.end(); it++)
            for (auto jt = next(it); jt != table.end(); jt++)
                nmp[prefix + it->first][jt->first] = intersection_merge(it->second, jt->second);
    }

    return nmp;
}

Data<int> hash_data(Data<string>& smp)
{
    Data<int> mp;

    for (auto& i : smp)
    {
        auto prefix = i.first;
        auto table = i.second;

        vector<int> nprefix;
        for (auto& s : prefix)
            nprefix.push_back(lower_bound(items_hash.begin(), items_hash.end(), s) - items_hash.begin());
        auto& p = mp[nprefix];
        for (auto& j : table)
        {
            auto suffix = j.first;
            auto TS = j.second;
            auto& ps = p[lower_bound(items_hash.begin(), items_hash.end(), suffix) - items_hash.begin()];
            for (auto& t : TS)
                ps.push_back(lower_bound(trans_hash.begin(), trans_hash.end(), t) - trans_hash.begin());
        }
    }
    return mp;
}

Data<string> de_hash_data(Data<int>& mp)
{
    Data<string> smp;

    for (auto& i : mp)
    {
        auto nprefix = i.first;
        auto p = i.second;

        vector<string> prefix;
        for (auto index : nprefix)
            prefix.push_back(items_hash[index]);

        auto& table = smp[prefix];
        for (auto& j : p)
        {
            auto suffix_index = j.first;
            auto ps = j.second;
            string suffix = items_hash[suffix_index];
            vector<string> TS;
            for (auto t_index : ps)
                TS.push_back(trans_hash[t_index]);
            table[suffix] = TS;
        }
    }
    return smp;
}

void print_all_frequent_item_sets(vector<Data<string>>& S_L)
{

    cout << "\n=========================================\n";
    cout << "          All Frequent Item Sets\n";
    cout << "=========================================\n{\n";
    vector<vector<string>> all_frequent_items;
    int L = 1;
    for (auto& l : S_L)
    {
        cout << "L" << L++ << ":\n";
        for (auto& i : l)
        {
            auto& prefix = i.first;
            auto& table = i.second;
            for (auto& j : table)
            {
                auto& suffix = j.first;
                auto& TS = j.second;
                cout << "\t" << prefix + suffix << " :\t{ " << TS << "}\n", all_frequent_items.push_back(prefix + suffix);
            }
        }

    }
    cout << "\n\n\nAll frequent items :-\n";
    for (auto i : all_frequent_items)
        cout << "\t{ " << i << "}\n";


    cout << "}\n=========================================\n";
    cout << "          End of Frequent Item Sets\n";
    cout << "=========================================\n";
}

void calculate_support_count(vector<Data<string>> S_L, map<vector<string>, long double>& support_count)
{
    for (auto l : S_L)
        for (auto i : l)
        {
            auto prefix = i.first;
            auto table = i.second;
            for (auto j : table)
            {
                auto suffix = j.first;
                auto TS = j.second;
                support_count[prefix + suffix] = (int)TS.size();
            }
        }
}

void represent_association_rules(vector<Data<string>> S_L, long double minimum_confidence, map<vector<string>, long double>& support_count, int& NumberOfAllTransactions)
{

    cout << "\n=========================================\n";
    cout << "Association Rules for Frequent Items and Lift\n";
    cout << "==========================================\n{\n";

    vector<vector<string>> frequent_item_sets;
    for (auto& l : S_L)
    {
        for (auto& i : l)
        {
            auto& prefix = i.first;
            auto& table = i.second;
            {
                for (auto& j : table)
                {
                    auto& suffix = j.first;
                    auto& TS = j.second;
                    frequent_item_sets.push_back(prefix + suffix);
                }
            }
        }

    }
    vector<Rule> rule, strong_rule;
    for (auto& fs : frequent_item_sets)
    {
        if (fs.size() == 1)
            continue;
        cout << fixed << setprecision(0);
        cout << '\n' << fs << ":\t\tsupport count = " << support_count[fs] << " :-\n";
        cout << fixed << setprecision(7);
        int i = 1;
        for (bitset<32> mask = i; i < (1LL << fs.size()) - 1; i++, mask = i)
        {
            rule.push_back(Rule());
            for (int j = 0; j < fs.size(); j++)
                if (mask[j])
                    rule.back().right.push_back(fs[j]);
                else
                    rule.back().left.push_back(fs[j]);

            auto& r = rule.back();

            const double EPS = (1e-8);
            auto dcmp = [&](double x, double y) {return fabs(x - y) <= EPS ? 0 : x < y ? -1 : 1; };

            long double confidence = support_count[fs] / support_count[r.left], lift = confidence / support_count[r.right] * NumberOfAllTransactions;
            bool strong = confidence >= minimum_confidence;

            cout << "\t" << r.left << "--> " << r.right << "\t\tconfidence = " << confidence << "\t\tlift = " << lift << "\t" << (~dcmp(lift, 1) ? !dcmp(lift, 1) ? "\"independent\"\t" : "\"dependent, +ve correlated\"\t" : "\"dependent, -ve correlated\"\t") << (strong ? "\t\"Strong Rule\"\n" : "\n");
            if (strong)
                strong_rule.push_back(r);
        }
    }
    cout << "\n}\n=========================================\n";
    cout << "     End of Association Rules and Lift\n";
    cout << "=========================================\n";


    cout << "\n=========================================\n";
    cout << "              Strong Rules\n";
    cout << "==========================================\n{\n";

    for (auto r : strong_rule)
        cout << r.left << "--> " << r.right << "\n";

    cout << "}\n=========================================\n";
    cout << "         End of Strong Rules\n";
    cout << "==========================================\n\n";

}


void Run_ECLAT()
{

    Data<string> smp;
    cout << "=========================================\n";
    cout << "              ECLAT Algorithm\n";
    cout << "=========================================\n";

    cout << "Enter the minimum support count [Not Percentage]\n";
    cin >> minSup;
    cout << "Enter the minimum confidence value [Available Range 0.0 -> 1.0]\n";
    cin >> minConf;

    string fileName = "my_file.csv";
    cout << "Enter the file name\n";
    cout << "(Make Sure the file name is something like \"" << fileName << "\" without the quotes and make sure that it is a csv file that doesn't use the UTF [more in the attached PDF]" << ")\n";
    cin.ignore();
    getline(cin, fileName);

    FileIterator fileIt(fileName);
    while (fileIt.hasNext())
    {
        string item = fileIt.getCurrItem();          //returns a string that represents the item
        auto& c1 = smp[{}][item];
        items_hash.push_back(item);
        vector<string> currItemTransactions = fileIt.getCurrItemTransactions();//returns a vector string that represents the transactions for the item
        for (string transactionId : currItemTransactions)
        {
            trans_hash.push_back(transactionId);
            c1.push_back(transactionId);
        }
        sort(c1.begin(), c1.end());
    }

    sort(trans_hash.begin(), trans_hash.end());
    trans_hash.erase(unique(trans_hash.begin(), trans_hash.end()), trans_hash.end());

    Data<int> mp;
    vector<Data<int>> N_L;

    mp = hash_data(smp);
    CtoL(mp);
    while (mp.size())
    {
        N_L.push_back(mp);
        mp = next_C_table(mp);
        CtoL(mp);
    }

    vector<Data<string>> S_L;
    for (auto n_l : N_L)
    {
        Data<string> sd = de_hash_data(n_l);
        S_L.push_back(sd);
    }


    map<vector<string>, long double> support_count;
    calculate_support_count(S_L, support_count);
    print_all_frequent_item_sets(S_L);
    int sz = trans_hash.size();
    represent_association_rules(S_L, minConf, support_count, sz);

    cout << "=========================================\n";
    cout << "          End of ECLAT Algorithm\n";
    cout << "=========================================\n";

}

signed main()
{

    Run_ECLAT(), cout << '\n';
    return 0;
}

/*

2 0.5 5
I1 6 100 400 500 700 800 900
I2 7 100 200 300 400 600 800 900
I3 6 300 500 600 700 800 900
I4 2 200 400
I5 2 100 800

*/