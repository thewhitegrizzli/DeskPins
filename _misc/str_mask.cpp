#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
#include <algorithm>
using namespace std;


// Wildcard compare.
// From CodeProject by Jack Handy (May 2 2001), with minor changes.
// 
bool wildcmp(const char* wild, const char* string) {
	const char *cp, *mp;
	
	while ((*string) && (*wild != '*')) {
		if ((*wild != *string) && (*wild != '?')) {
			return false;
		}
		wild++;
		string++;
	}
		
	while (*string) {
		if (*wild == '*') {
			if (!*++wild) {
				return true;
			}
			mp = wild;
			cp = string+1;
    }
    else if ((*wild == *string) || (*wild == '?')) {
			wild++;
			string++;
    }
    else {
			wild = mp;
			string = cp++;
		}
	}
		
	while (*wild == '*') {
		wild++;
	}
	return !*wild;
}


void trim(string& s) {
  string::size_type i;

  if (s.empty()) return;
  i = s.find_first_not_of(" ");
  if (i != string::npos) s.erase(0, i);

  if (s.empty()) return;
  i = s.find_last_not_of(" ");
  if (i != string::npos) s.erase(i+1);
}


istream& myGetLine(istream& is, string& s) {
  char buf[100];
  is.getline(buf, sizeof(buf));
  if (is)
    s = buf;
  return is;
}


class Masker {
public:
  class token {
  public:
    bool wild;    // true if this is a *
    int  any;     // >0 if this a string of ?'s
    string txt;  // literal text, if non of the above

    token()                : wild(true),  any(0)         {}
    token(int n)           : wild(false), any(n)         {}
    token(const string& s) : wild(false), any(0), txt(s) {}

    bool isWild() const {return txt.empty();}
  };

  static const token wildToken;

  vector<token> tokens;


  Masker(const string& mask) {
    if (mask.empty())
      return;   // create empty obj; matches only an empty string

    // tokenize (one char at a time)
    for (string::size_type i = 0; i < mask.length(); ++i) {
      switch (mask[i]) {
        case '*':
          // add *-wildcard, if there's not one already at the end
          if (tokens.empty() || !tokens.back().wild)
            tokens.push_back(token());
          break;
        case '?':
          // add ?-wildcard, or increment last (if any)
          if (tokens.empty() || !tokens.back().any)
            tokens.push_back(token(1));
          else
            ++tokens.back().any;
          break;
        default:
          // add new TXT token, or append to existing at the end
          if (tokens.empty() || tokens.back().txt.empty())
            tokens.push_back(string(1, mask[i]));
          else
            tokens.back().txt += mask[i];
          break;
      }
    }


    cout << '\t' << "Masker inited:" << endl;
    for (int n = 0; n < tokens.size(); ++n) {
      const token& tok = tokens[n];
      if (tok.wild)
        cout << "\t\t*" << endl;
      else if (tok.any)
        cout << "\t\t? x " << tok.any << endl;
      else
        cout << "\t\tTXT " << tok.txt << endl;
    }

    // simplify sequence's of adjacent * and ?
    // - remove multiple *
    // - concatenate multiple ?
    vector<token>::iterator itBeg, itEnd;
    itBeg = tokens.begin();
    while (getTokSeq(itBeg, itEnd)) {
      // There should always be a '*' token,
      // otherwise, the '?' tokens wouldn't be separate.
      // So, we just have to accumulate the '?' token lengths
      int total = 0;
      for (vector<token>::const_iterator it = itBeg; it != itEnd; ++it)
        total += it->any;    // 'any' will be 0 if this is an '*' token

      // remove the original sequence
      tokens.erase(itBeg, itEnd);
      // insert the simplified tokens (order doesn't really matter)
      tokens.insert(itBeg, token());
      tokens.insert(itBeg, token(total));
      // setup beg past this modified sequence
      itBeg += 2;
    }

    cout << '\t' << "After simplification:" << endl;
    for (n = 0; n < tokens.size(); ++n) {
      const token& tok = tokens[n];
      if (tok.wild)
        cout << "\t\t*" << endl;
      else if (tok.any)
        cout << "\t\t? x " << tok.any << endl;
      else
        cout << "\t\tTXT " << tok.txt << endl;
    }

  }

  static bool isNonTextToken(const token& tok) {
    return tok.txt.empty();
  }

  static bool isTextToken(const token& tok) {
    return !tok.txt.empty();
  }

  // IN
  //    itBeg:  specifies item to start from
  // OUT
  //    ret -> found sequence
  //    itBeg:  beginning of found sequence
  //    itEnd:  end of found sequence
  bool getTokSeq(vector<token>::iterator& itBeg, vector<token>::iterator& itEnd) {
    for (;;) {
      // find first non-text token
      itBeg = find_if(itBeg, tokens.end(), isNonTextToken);
      if (itBeg == tokens.end())
        return false;

      // find next text token
      itEnd = find_if(itBeg, tokens.end(), isTextToken);

      if (itEnd - itBeg > 2)
        return true;

      itBeg = itEnd;
    }
  }

  bool match(const string& s) const {
    /*
    // an empty mask matches only an empty string -- elegant ;)
    if (tokens.empty())
      return s.empty();

    vector<token>::const_iterator curTok = tokens.begin();
    vector<token>::const_iterator endTok = tokens.end();

    string::size_type i = 0;

    .........
    .........
    ......... goodnight :))))
    */
    return true;
  }

};


/*
void doMask(const string& maskStr) {

  Masker mask(maskStr);
  
  return;

  string s;
  for (;;) {
    cout << "> ";
    if (!myGetLine(cin, s)) {
      cerr << "ERR: getline() failed" << endl;
      return;
    }
    //trim(s);
    if (s.empty())
      return;
    cout << (mask.match(s) ? "YES" : "NO") << endl;
  }

}
*/


void doMask(const string& maskStr) {

  string s;
  for (;;) {
    cout << "> ";
    if (!myGetLine(cin, s)) {
      cerr << "ERR: getline() failed" << endl;
      return;
    }
    if (s.empty())
      return;
    cout << wildcmp(maskStr.c_str(), s.c_str()) << endl;
  }

}



void test() {
  string s;

  for (;;) {
    cout << "Enter mask (nul to quit): ";
    if (!myGetLine(cin, s)) {
      cerr << "ERR: getline() failed" << endl;
      return;
    }
    trim(s);
    if (s.empty())
      return;
    doMask(s);
  }

}


int main(int argc, char* argv[])
{
  test();
	return 0;
}


/*

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <list>

// <memory>

int main() {
  srand((unsigned)time(0));

  const int len = 33000;

  list<int> lst;
  int n;
  for (n = 0; n < len; ++n)
    lst.push_back(rand());

  lst.sort();

  list<int>::const_iterator itPrev = lst.begin();
  list<int>::const_iterator it = lst.begin();
  list<int>::const_iterator end = lst.end();
  ++it;
  bool ok = true;
  for (; it != end; ++it, ++itPrev)
    if (*itPrev > *it) {
      ok = false;
      cerr << "error at index " << '?' << endl;
    }

  if (ok)
    cout << "OK" << endl;

  system("pause");

  return 0;
}
*/


/*
*qwe???asd*

ASDqwe123243qweXXX


12x34x34

12*34
*/