package org.metagarfus.lib.parse;

/**
* Created by miguel on 04-10-2014.
*/
class BytesString
{
  byte[] data;
  int start, end;

  BytesString()
  {
    this("");
  }

  BytesString(byte[] data, int start, int end)
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
