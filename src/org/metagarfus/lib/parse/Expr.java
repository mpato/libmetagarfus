package org.metagarfus.lib.parse;

/**
 * Created by msilva on 10/3/14.
 */
public class Expr
{
  public static final byte PLUS = '+';
  public static final byte MINUS = '-';
  public static final byte MUL = '*';
  public static final byte DIV = '/';
  public static final byte POW = '^';
  public static final byte EQUAL = '=';
  public static final byte GREATER = '>';
  public static final byte GREATER_EQUAL = 'g';
  public static final byte LESS = '<';
  public static final byte LESS_EQUAL = 'l';
  public static final byte OR = '|';
  public static final byte AND = '&';

  static final private byte[] reserved = {PLUS, MINUS, MUL, DIV, POW, EQUAL, GREATER, LESS, OR, AND, '(', ')'};

  public static class UnaryOp extends Expr
  {
    public byte operation;
    public Expr operand;
  }

  public static class BinaryOp extends Expr
  {
    public byte operation;
    public Expr operand1, operand2;
  }

  public static class Value extends Expr
  {
    public String value;
    public Object opaque;
  }

  public static boolean containsReservedCharacters(String str)
  {
    byte[] array;
    array = str.getBytes();
    for (byte b : array) {
      for (byte b2 : reserved) {
        if (b == b2)
          return true;
      }
    }
    return false;
  }

  public Expr parse(String str)
  {
    return null;
  }
}
