package org.metagarfus.tartan;

import org.metagarfus.lib.parse.TagTuple;

/**
 * Created by msilva on 9/25/14.
 */
public class Main
{
  public static void main(String args[])
  {
    try {
      System.out.println("Result: " + TagTuple.parse("  k:  o(l:1,(2),3(\"90\"),2:7, c(3))"));
      System.out.println("Result: " + TagTuple.parse("  k:  o(  \"or\\ta\", l:1,(2),3(\"90\"),2:7, c(3), \"ola\")"));
      System.out.println("Result: " + TagTuple.parse("3\"ola\""));
    } catch (Exception e) {
      e.printStackTrace();
    }
  }
}
