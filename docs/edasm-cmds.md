# EDASM Commands

| Command Syntax                                             | Command Number |
|------------------------------------------------------------|----------------|
| ASM PATHNAME , &lt;OBJ-PATHNAME&gt;                        | 0              |
| Add &lt;LINE#&gt;                                          | 2              |
| MON                                                        | 4              |
| PreFiX &lt;PATHNAME&gt;                                    | 6              |
| DELETE PATHNAME                                            | 8              |
| LOCK PATHNAME                                              | 10             |
| UNLOCK PATHNAME                                            | 12             |
| CAT &lt;PATHNAME&gt;                                       | 14             |
| LOaD PATHNAME                                              | 16             |
| SaVE &lt;BEGIN# &lt;-END#&gt;&gt; &lt;PATHNAME&gt;         | 18             |
| Insert LINE#                                               | 20             |
| Del &lt;BEGIN# &lt;-END#&gt;&gt;                           | 22             |
| SWAP                                                       | 24             |
| KILL2                                                      | 26             |
| NEW                                                        | 28             |
| Tabs &lt;BEGIN# &lt;-END#&gt;&gt; .STRING.                 | 30             |
| RENAME OLDPATHNAME , NEWPATHNAME                           | 32             |
| COpy LINE# - &lt;LINE#&gt; TO LINE#                        | 34             |
| BLOAD PATHNAME , A&lt;&#36;&gt;ADRS                        | 36             |
| List &lt;BEGIN# &lt;-END#&gt;&gt;                          | 38             |
| Change &lt;BEGIN# &lt;-END#&gt;&gt; .OLDSTR.NEWSTR.        | 40             |
| Find &lt;BEGIN# &lt;-END#&gt;&gt; .STRING.                 | 42             |
| Print &lt;BEGIN# &lt;-END#&gt;&gt;                         | 44             |
| Edit &lt;BEGIN# &lt;-END#&gt;&gt; .STRING.                 | 46             |
| APPEND &lt;LINE#&gt; PATHNAME                              | 48             |
| Replace &lt;BEGIN# &lt;-END#&gt;&gt;                       | 50             |
| FILE                                                       | 52             |
| PR# # , &lt;DEVCTL&gt;                                     | 54             |
| SETDelim .STRING.                                          | 56             |
| SETLcase                                                   | 58             |
| SETUcase                                                   | 60             |
| TRuncOFf                                                   | 62             |
| TRuncON                                                    | 64             |
| Where LINE#                                                | 66             |
| COLumn #                                                   | 68             |
| PTRON                                                      | 70             |
| PTROFF                                                     | 72             |
| Online                                                     | 74             |
| EXIT &lt;PATHNAME&gt;                                      | 76             |
| BSAVE PATHNAME , A&lt;&#36;&gt;ADRS , L&lt;&#36;&gt;LGTH   | 78             |
| CATALOG &lt;PATHNAME&gt;                                   | 80             |
| EXEC PATHNAME                                              | 82             |
| XLOAD PATHNAME , [A&lt;&#36;&gt;ADRS]                      | 84             |
| XSAVE PATHNAME , [A&lt;&#36;&gt;ADRS , L&lt;&#36;&gt;LGTH] | 86             |
| TYPE PATHNAME                                              | 88             |
| CREATE PATHNAME                                            | 90             |
| END                                                        | 92             |
