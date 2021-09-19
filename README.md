# mesgpack

messagepack の調査用ツール

## messagePack spec.

https://github.com/msgpack/msgpack/blob/master/spec.md

## chunk I/O Spec.

https://github.com/fluent/fluent-bit/tree/37aa680d32384c1179f02ee08a5bef4cd278513e/lib/chunkio

## NULL Block
レコードが終了したのち、先頭から NULL バイトが並んでいる場合、NULL バイトブロックのサイズを表示する。NULLバイトのバイト数とNULL バイトが出現した場所のオフセット(ファイル名を含む先頭のchunkヘッダが終わった場所からのオフセット)を表示している。
