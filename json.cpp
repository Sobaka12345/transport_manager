#include "json.h"
#include <iostream>

using namespace std;
namespace Json {

  Document::Document(Node root) : root(move(root)) {
  }

  const Node& Document::GetRoot() const {
    return root;
  }

  Node LoadNode(istream& input);

  Node LoadArray(istream& input) {
    vector<Node> result;

    for (char c; input >> c && c != ']'; ) {
      if (c != ',') {
        input.putback(c);
      }
      result.push_back(LoadNode(input));
    }

    return Node(move(result));
  }

  Node LoadNumeric(istream& input) {
    string num;
    while(isdigit(input.peek()) || input.peek() == '.' || input.peek() == '-')
    {
        char next;
        input.get(next);
        num.push_back(next);
    }

    if(num.find('.') != num.npos)
        return Node(stod(num));
    else
        return Node(stoi(num));
  }

  Node LoadBoolean(istream & input)
  {
      string boolean;
      while(input.peek() >= 'a' && input.peek() <= 'z')
      {
          char next;
          input.get(next);
          boolean.push_back(next);
      }

      if(boolean.find("true") != boolean.npos)
          return Node(true);
      else
          return Node(false);
  }

  Node LoadString(istream& input) {
    string line;
    getline(input, line, '"');
    return Node(move(line));
  }

  Node LoadDict(istream& input) {
    map<string, Node> result;

    for (char c; input >> c && c != '}'; ) {
      if (c == ',') {
        input >> c;
      }

      string key = LoadString(input).AsString();
      input >> c;
      result.emplace(move(key), LoadNode(input));
    }

    return Node(move(result));
  }

  Node LoadNode(istream& input) {
    char c;
    input >> c;

    if (c == '[') {
      return LoadArray(input);
    } else if (c == '{') {
      return LoadDict(input);
    } else if (c == '"') {
      return LoadString(input);
    } else if(isdigit(c) || c == '-'){
      input.putback(c);
      return LoadNumeric(input);
    } else {
      input.putback(c);
      return LoadBoolean(input);
    }
  }

  Document Load(istream& input) {
    return Document{LoadNode(input)};
  }

}
