package org.metagarfus.lib.parse;

import java.util.Vector;

public abstract class TagTuple
{
  public String exprTag;

  public static class Tuple extends TagTuple
  {
    public String typeTag;
    public Vector<TagTuple> args;

    public Tuple(String exprTag, String typeTag, Vector<TagTuple> args)
    {
      this.exprTag = exprTag;
      this.typeTag = typeTag;
      this.args = args;
    }

    public String toString()
    {
      String ret;
      ret = "( name : " + exprTag + " type: " + typeTag + " args: ";
      for (TagTuple tagTuple : args)
        ret += "(" +  tagTuple +")";
      return ret + " )";
    }
  }

  public static class Value extends TagTuple
  {
    public String value;
    public Object opaque;

    public Value(String exprTag, String value)
    {
      this.exprTag = exprTag;
      this.value = value;
    }

    public String toString()
    {
      return "( name : " + exprTag + " value: " + value + " )";
    }
  }

  public static class StringValue extends Value
  {
    public StringValue(String exprTag, String value)
    {
      super(exprTag, value);
    }

    public String toString()
    {
      return "( name : " + exprTag + " string: " + value + " )";
    }
  }

  private static boolean isString(BytesString str)
  {
    return str.length() >= 1 && str.charAt(0) == '"';
  }

  private static boolean isString(String str)
  {
    return str.length() >= 1 && str.charAt(0) == '"';
  }

  private static boolean isWhiteSpace(byte tchar)
  {
    return tchar == ' ' || tchar == '\t' || tchar == '\n';
  }

  private static class Parsed
  {
    TagTuple result;
    String token;
    char nextTChar;
    BytesString remaining;
  }

  private static class BytesString
  {
    byte[] data;
    int start, end;

    private BytesString()
    {
      this("");
    }

    private BytesString(byte[] data, int start, int end)
    {
      this.data = data;
      this.start = start;
      this.end = end;
    }

    BytesString(String data)
    {
      this(data.getBytes(), 0, 0);
      end = this.data.length;
    }

    int length()
    {
      return end - start;
    }

    byte charAt(int i)
    {
      return data[start + i];
    }

    BytesString substring(int start, int end)
    {
      return new BytesString(data, this.start + start, this.start + end);
    }

    BytesString substring(int start)
    {
      return substring(start, length());
    }

    public String toString()
    {
      return new String(data).substring(start, end);
    }
  }

  private static void consumeString(BytesString source, Parsed tk)
  {
    boolean escaped;
    String ret;
    int j;
    byte tchar;
    ret = "";
    j = 0;
    escaped = false;
    for (int i = 1; i < source.length(); i++) {
      tchar = source.charAt(i);
      if (!escaped) {
        if (tchar == '"') {
          tk.token = ret + source.substring(j, i + 1);
          consumeWhiteSpace(i + 1, source, tk);
          return;
        } else if (tchar == '\\') {
          escaped = true;
          ret = ret + source.substring(j, i);
          j = i + 1;
        }
      } else {
        escaped = false;
      }
    }
    tk.token = source.toString();
    tk.nextTChar = '\0';
    tk.remaining = new BytesString();
  }

  private static void consume(BytesString source, Parsed tk)
  {
    int j, t;
    String ret;
    byte tchar;
    if (isString(source)) {
      consumeString(source, tk);
      return;
    }
    j = 0;
    ret = "";
    for (int i = 0; i < source.length(); i++) {
      tchar = source.charAt(i);
      if (tchar == '(' || tchar == ')' || tchar == ',' || tchar == ':') {
        if (i + 1 >= source.length()) {
          t = source.length();
        } else {
          t = i + 1;
        }
        tk.token = ret + source.substring(j, i);
        tk.nextTChar = (char) tchar;
        tk.remaining = source.substring(t);
        return;
      } else if (isWhiteSpace(tchar)) {
        ret = ret + source.substring(j, i);
        j = i + 1;
      }
    }
    tk.token = source.toString();
    tk.nextTChar = '\0';
    tk.remaining = new BytesString();
  }

  private static void consumeWhiteSpace(int start, BytesString source, Parsed tk)
  {
    int t;
    byte tchar;

    for (int i = start; i < source.length(); i++) {
      tchar = source.charAt(i);
      if (!isWhiteSpace(tchar)) {
        if (i + 1 >= source.length()) {
          t = source.length();
        } else {
          t = i + 1;
        }
        tk.nextTChar = (char) tchar;
        tk.remaining = source.substring(t);
        return;
      }
    }
    tk.nextTChar = '\0';
    tk.remaining = new BytesString();
  }

  private static void parseString(String exprTag, String r, char s, BytesString source, Parsed tt)
  {
    tt.result = new StringValue(exprTag, r.substring(1, r.length() - 1));
    tt.nextTChar = s;
    tt.remaining = source;
  }

  private static void parse(BytesString source, Parsed tk) throws Exception
  {
    String exprTag;
    Tuple tuple;
    exprTag = "";
    consume(source, tk);
    if (isString(tk.token)) {
      parseString(exprTag, tk.token, tk.nextTChar, tk.remaining, tk);
      return;
    }
    if (tk.nextTChar == ':') {
      exprTag = tk.token;
      consume(tk.remaining, tk);
      if (isString(tk.token)) {
        parseString(exprTag, tk.token, tk.nextTChar, tk.remaining, tk);
        return;
      }
    }
    if (tk.token != null && tk.token.length() > 0 && (tk.nextTChar == ')' || tk.nextTChar == ',')) {
      tk.result = new Value(exprTag, tk.token);
    } else if (tk.nextTChar == '(') {
      tuple = new Tuple(exprTag, tk.token, new Vector<TagTuple>());
      while (tk.nextTChar != ')') {
        parse(tk.remaining, tk);
        if (tk.nextTChar != ',' && tk.nextTChar != ')') {
          throw new Exception("Unexpected symbol " + (char) tk.nextTChar);
        }
        if (tk.result == null) {
          if (tuple.args.size() == 0 && tk.nextTChar == ')')
            continue;
          throw new Exception("Empty argument");
        }
        tuple.args.add(tk.result);
      }
      tk.result = tuple;
      consumeWhiteSpace(0, tk.remaining, tk);
    } else {
      tk.result = null;
    }
  }

  public static TagTuple parse(String source) throws Exception
  {
    BytesString btSource;
    Parsed tk;
    btSource = new BytesString(source);
    tk = new Parsed();
    parse(btSource, tk);
    if (tk.nextTChar != '\0')
      throw new Exception("Unexpected symbol " + (char) tk.nextTChar);
    return tk.result;
  }
}
