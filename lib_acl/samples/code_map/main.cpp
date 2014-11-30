#include "lib_acl.h"

#ifdef WIN32
#define snprintf _snprintf
#endif

#define TOTAL_CHAR		2359
#define TOTAL_CHAR_BUF	4724

static char __szJJ[TOTAL_CHAR_BUF]=
"³¡´¡·¡¾¡À¡Ã¡Å¡É¡Ì¡Ñ¡Ò¡Õ¡Ö¡Ý¡ç¡é¡ê¡í¡ï¡ò¡÷¡²¢³¢´¢·¢¼¢½¢¾¢Â¢Å¢Ì¢Ñ¢Ò¢Õ¢Ö¢×¢ß¢ã¢ç¢ê¢ï¢ò¢÷¢µ£·£¼£½£¾£À£Â£"
"Ç£È£Ò£Ó£Ö£Û£Ý£ß£å£æ£ç£ê£ì£ï£ð£ò£ö£÷£³¤¶¤»¤½¤Á¤Â¤Ç¤Ê¤Î¤Ñ¤Ò¤Ó¤Ô¤Õ¤Ö¤×¤Ý¤â¤ä¤ç¤é¤ê¤ï¤ò¤÷¤±¥³¥´¥µ¥¶¥¼¥½¥"
"¾¥¿¥Á¥Â¥Å¥Ç¥É¥Ê¥Î¥Ð¥Ò¥Ó¥Ú¥Ý¥ß¥á¥ã¥ä¥ç¥ê¥ï¥ò¥ô¥÷¥±¦²¦³¦´¦µ¦»¦¼¦½¦Â¦Å¦Ç¦Ê¦Ó¦Ô¦Ú¦Û¦Ý¦ß¦ç¦ï¦ò¦õ¦ö¦÷¦²§³§"
"µ§¶§·§¸§º§»§½§Â§Å§È§É§Ë§Ì§Î§Ñ§Ó§Ô§Ú§Û§Ý§ç§ê§ë§ï§ò§õ§÷§°¨±¨µ¨¸¨¹¨¼¨¾¨Â¨Ã¨Å¨Ç¨È¨É¨Ê¨Ì¨Ó¨Ô¨×¨Ø¨Ú¨â¨ç¨è¨"
"ê¨í¨ï¨ñ¨ò¨ö¨÷¨³©¶©·©»©¼©À©Á©Å©Ç©Ò©Ó©Õ©×©Ú©Û©æ©ç©ì©ï©ò©÷©°ª¶ª»ª¾ª¿ªÁªÃªÊªÎªÓª×ªÚªÛªÝªçªëªìªíªïªòª÷ª±«"
"´«¶«º«¼«½«À«Á«Â«Ç«Ê«Ë«Î«Ð«Ñ«Ó«Õ«Ú«Ý«á«ã«ä«æ«ç«ì«ï«ñ«ò«ö«÷«²¬µ¬¹¬½¬Á¬Â¬É¬Ê¬Ì¬Î¬Ó¬×¬Ú¬æ¬ç¬è¬ì¬ï¬ò¬÷¬°­"
"»­¼­¾­¿­Á­Â­Ã­Æ­Ë­Í­Î­Ð­Ù­Ú­Þ­á­ç­é­ì­ï­ò­÷­°®³®µ®¹®»®Â®Æ®Ç®Ï®Ð®Ó®Õ®×®Ú®á®æ®ç®ì®ï®ñ®÷®´¯µ¯¶¯·¯½¯À¯Á¯"
"Â¯Ç¯Ì¯Ð¯Ñ¯Ò¯Ô¯Ö¯×¯Ù¯Ú¯Ý¯ä¯ç¯ê¯í¯ï¯ð¯ô¯÷¯¶°·°»°½°À°Á°Â°È°Ë°Ì°Î°Ï°Ñ°Ô°Ö°×°Ú°Ý°á°ä°å°ç°í°ï°ð°ö°µ±¹±½±¾±"
"Á±Â±Ä±Å±Ç±É±Ê±Ì±Î±Ñ±Ó±Ô±Õ±×±Ù±Ú±ß±ä±ç±ê±ì±ï±ð±ò±±²µ²½²¾²Á²Â²É²Ì²Ð²Ô²Ù²Ú²Ý²ç²ë²ï²ð²ñ²ò²÷²´³µ³¶³·³¸³¹³"
"»³À³Á³Â³Ã³Ç³Ë³Ì³Í³Î³Ï³Ð³Ò³×³Ù³Ú³ã³ç³ï³ð³ñ³ö³÷³±´´´µ´¸´½´À´Á´Ã´Ç´È´É´Ê´Ð´Ó´Ö´×´Ú´Þ´ß´á´ã´æ´ç´é´ì´í´î´"
"ï´ð´ö´±µ²µ³µµµ¹µ»µ¾µÀµÁµÅµÆµÇµÈµÊµËµÑµÒµÓµÔµÕµÚµÝµäµæµçµéµëµìµíµïµðµôµöµ÷µ·¶¹¶»¶¼¶¾¶À¶Á¶Ä¶Æ¶Ê¶Ë¶Ñ¶Ò¶"
"Ó¶Ô¶Õ¶×¶Ù¶Ú¶à¶ã¶ä¶ç¶è¶í¶ï¶ð¶ö¶±·²·µ·¶···»·¼·¾·Á·Å·È·Ì·Í·Ï·Ñ·Õ·Ú·à·ç·é·ï·ð·ö·±¸´¸¼¸À¸Á¸Â¸Å¸É¸Ë¸Ì¸Ï¸Ó¸"
"Ô¸Õ¸×¸Ú¸ß¸å¸ç¸è¸í¸ï¸ð¸ö¸°¹±¹²¹³¹·¹¹¹»¹À¹Á¹Ã¹Å¹Ç¹É¹Î¹Ñ¹×¹Ú¹å¹ç¹è¹ë¹ï¹ð¹ò¹ö¹÷¹µº¸º¹ººº»º½º¾ºÀºÇºÍºÏºÐº"
"×ºØºÚºÝºÞºçºèºíºïºðºòºõºöºµ»¹»»»¼»¾»À»Â»Ã»Å»Æ»Ê»Ð»Ñ»Ó»Õ»×»Ú»Û»Ý»Þ»á»ç»ï»ð»õ»ö»µ¼¸¼À¼Â¼Í¼Ñ¼Ô¼×¼Ú¼Û¼Ü¼"
"ß¼â¼ç¼ê¼î¼ï¼ð¼ñ¼ö¼»½½½À½Á½Â½Å½Ç½Î½Ï½Ò½Ó½Õ½Ö½Ú½Û½ß½á½â½ç½ï½ð½ö½÷½³¾¸¾»¾½¾À¾Á¾Ã¾Æ¾Ç¾É¾Ì¾Ô¾Ö¾Ú¾â¾ç¾ì¾ï¾"
"ð¾ö¾°¿´¿¶¿¸¿½¿À¿Â¿Ç¿Ë¿Í¿Ï¿Ð¿Ò¿Ó¿Ô¿Ö¿Ú¿Ü¿ß¿à¿á¿â¿æ¿ç¿è¿ì¿í¿î¿ï¿ð¿ó¿°À¶À»À¾ÀÀÀÂÀÆÀÇÀÌÀÎÀÏÀÔÀÕÀÖÀÚÀáÀâÀ"
"ãÀçÀèÀéÀìÀïÀðÀñÀ±Á³ÁµÁ¶Á»Á¼Á½ÁÀÁÂÁÉÁÏÁÔÁÚÁÞÁáÁâÁãÁæÁçÁìÁïÁðÁôÁöÁ°Â³Â´Â½ÂÀÂÁÂÂÂÇÂÉÂÎÂØÂÚÂÛÂÜÂßÂâÂäÂçÂ"
"íÂïÂðÂ·Ã¸Ã¼Ã½ÃÀÃÆÃÈÃÏÃÒÃÔÃÚÃßÃâÃçÃíÃïÃðÃòÃ÷Ã³Ä¶Ä·Ä½ÄÀÄÆÄÈÄÉÄÔÄÖÄØÄÚÄßÄâÄçÄéÄïÄðÄõÄ³ÅºÅ½Å¿ÅÀÅÂÅÃÅÄÅÇÅ"
"ÈÅÍÅÎÅÏÅÒÅÓÅÕÅ×ÅÚÅâÅãÅäÅçÅèÅîÅïÅðÅöÅ÷Å²Æ³ÆµÆ¶Æ¸Æ¹Æ»Æ¼ÆÁÆÂÆÃÆÄÆÈÆÊÆÎÆÐÆÑÆÔÆÚÆÞÆâÆãÆçÆéÆîÆïÆðÆóÆ÷Æ´Ç¸Ç"
"¼Ç¾Ç¿ÇÂÇÃÇÇÇËÇÍÇÏÇÑÇÒÇÓÇÔÇÕÇ×ÇØÇÙÇÚÇßÇâÇãÇåÇçÇèÇìÇîÇïÇðÇôÇöÇ½ÈÇÈÈÈÎÈÑÈÔÈ×ÈÚÈÜÈàÈâÈãÈæÈçÈèÈîÈïÈðÈóÈõÈ"
"·É¸É½É¾ÉÁÉÄÉÉÉÍÉÔÉØÉÚÉÜÉáÉâÉãÉäÉåÉæÉçÉèÉéÉëÉîÉïÉðÉñÉòÉöÉ±Ê´Ê¼Ê½ÊÉÊÊÊËÊÎÊÏÊÓÊÕÊÖÊ×ÊÚÊÛÊÜÊâÊãÊçÊêÊëÊîÊ"
"ïÊðÊ÷ÊµË¸Ë¹ËÂËÆËÉËËËÏËÐËÑËÓËÔËÕËØËÚËâËãËäËçËëËìËîËïËðË÷Ë·Ì¼ÌÀÌÂÌÃÌÅÌÆÌÇÌËÌÌÌÏÌÑÌÓÌÔÌØÌÙÌÚÌßÌâÌãÌçÌíÌ"
"îÌïÌðÌòÌôÌ³Í´Í¶Í¼ÍÀÍÁÍÂÍÈÍÉÍÊÍÎÍÏÍÒÍÔÍÕÍÖÍØÍÙÍÚÍâÍãÍæÍçÍéÍêÍëÍíÍîÍïÍðÍñÍòÍ²Î½Î¿ÎÂÎÃÎÊÎÌÎÎÎÏÎÑÎÓÎÔÎÙÎ"
"ÚÎàÎáÎâÎãÎåÎçÎèÎìÎîÎïÎðÎõÎ±Ï²Ï³Ï´Ï¶Ï·Ï¸Ï½ÏÂÏÇÏÈÏËÏÎÏÑÏÒÏÔÏÙÏÚÏÛÏÞÏãÏçÏîÏïÏðÏñÏòÏôÏõÏöÏ÷Ï±Ð²Ð´ÐµÐ¶Ð¹Ð"
"¼ÐÂÐÃÐËÐÌÐÎÐÏÐØÐÙÐÚÐÜÐßÐáÐâÐãÐçÐèÐîÐïÐðÐñÐôÐöÐ÷Ð²Ñ´Ñ·Ñ¸Ñ»Ñ¿ÑÄÑÍÑÎÑØÑÚÑÛÑÜÑÞÑãÑçÑìÑîÑïÑðÑõÑöÑ±Ò²Ò³Ò¶Ò"
"¹ÒºÒ¿ÒÂÒÈÒÍÒÑÒÚÒÞÒãÒçÒîÒïÒðÒõÒöÒ°Ó²Ó´ÓµÓ¶Ó¸Ó»ÓÄÓÅÓÆÓÊÓËÓÔÓÖÓØÓÚÓÝÓàÓãÓäÓçÓèÓêÓíÓîÓïÓðÓòÓöÓ²Ô´Ô¶Ô¸Ô»Ô"
"¼Ô¾ÔÀÔÁÔÄÔÇÔÊÔÍÔÎÔÏÔÕÔÚÔÝÔßÔãÔçÔîÔïÔðÔöÔ°Õ±Õ²Õ³Õ´Õ¸Õ¼Õ½ÕÂÕÃÕÄÕÇÕÉÕËÕÍÕÏÕÑÕÒÕÓÕÖÕ×ÕÚÕßÕáÕãÕçÕîÕïÕðÕöÕ"
"²Ö¶Ö¸Ö¼ÖÀÖÂÖÃÖÄÖÌÖÍÖÏÖÑÖÔÖÖÖÚÖãÖæÖçÖéÖëÖìÖîÖïÖðÖóÖôÖöÖ²×·×º×½×Â×Å×Æ×Ç×Ï×Ð×Ö×Ú×Ü×ã×ç×í×î×ï×ð×ö×·Ø¹ØºØ"
"¼ØÀØÂØÎØÏØÔØÚØÞØßØãØçØîØïØðØöØ³Ù¶Ù¸Ù»Ù¾Ù¿ÙÁÙÂÙÃÙÄÙÈÙÊÙÎÙ×ÙØÙÚÙßÙàÙâÙãÙäÙçÙèÙîÙïÙðÙóÙõÙöÙ°Ú´Ú¸Ú½ÚÁÚÂÚ"
"ÄÚÌÚÎÚÏÚÐÚÒÚÓÚÖÚÚÚãÚçÚéÚêÚëÚîÚïÚñÚöÚ³Û¶Û¹Û¼ÛÁÛÂÛÎÛÏÛ×ÛØÛÚÛÛÛÝÛáÛãÛêÛîÛïÛöÛ°Ü³Ü´Ü·Ü½ÜÂÜÆÜÉÜÌÜÎÜÏÜÔÜ×Ü"
"ØÜÚÜÜÜßÜãÜäÜêÜîÜïÜðÜñÜõÜöÜ³ÝµÝ¹Ý¼Ý¾ÝÀÝÁÝÃÝÍÝÔÝÕÝ×ÝÙÝÚÝáÝèÝéÝêÝîÝïÝðÝóÝöÝ²ÞµÞ¸ÞÁÞÂÞÇÞÈÞÉÞÊÞÎÞÑÞÔÞÕÞ×Þ"
"ÚÞÛÞÝÞâÞäÞêÞíÞîÞïÞöÞ÷Þ±ßµß·ß¹ß»ß¼ßÂßËßÎßÏßÓßÔßÖßÚßèßêßîßïßðßñßöß±à²à·à»à¼à½àÀàÂàÃàÆàÊàËàÌàÍàÔàÕàÛàÜà"
"ßàààæàêàîàïàöà±á²á¶á·á¹áºá»á¼á½áÀáÂáÇáÏáÑáÖáÙáÚáæáçáêáëáîáïáöá÷á²â»â¾â¿âÂâÄâÅâÇâÌâÎâÏâÐâÑâÕâÛâÞâßâáâ"
"æâçâèâéâêâìâîâïâðâöâ²ã³ãµã·ã¹ã»ã¼ã¿ãÇãÉãÓãÜãâãããäãæãèãêãîãïãöã°ä±ä¸äºä»ä¼ä¿äÁäÊäËäÍäÒäÔäÙäÛäÜäÝäâäää"
"æäéäêäîäïäñäöä³å³å»å¾åÁåÃåÄåÉåÌåÍåÐåÑåÒåÖåæåçåèåêåîåïåðåòåóåöå³æµæ·æ¹æ»æÂæËæÓæÔæÙæßæâæææéæêæîæïæóæõæ"
"öæ±çµç·ç»ç¾çÂçÅçËçÍçÏçÖç×çæçççèçéçêçîçïçõçöç±è³è·è¹èºè¼èÁèÂèÉèÑèÒèÖèÞèàèáèæèêèíèîèïèöè¶é¸é¹é¾é¿éÀéÁé"
"ÊéÐéÑéÒéÓé×éÜéßéâéæéééêéîéïéðéõéöé±ê¹ê¼êÀêÂêÆêÇêÊêËêÏêÐêÒêÕê×êÚêÜêÞêâêæêêêîêïêóêôêöê³ë·ë¸ë¹ë»ë¼ë½ë¿ë"
"ÀëÁëÂëÆëÇëÌëÎëÐëÒëÓëÕëÛëÝëßëàëâëäëæëçëèëéëêëîëïëöë°ì³ì¶ì·ì¹ìºì¼ì½ìÁìÂìÄìÇìÏìÑìÒìÓìÕìÙìÞìäìæìêìîìïìðì"
"ñìóìöì°í´íµí·í¿íÂíÃíÄíÇíÈíÎíÐíÖíÚíãíäíæíèíéíêíîíïíöí±î¶î¹î¼î¾îÂîÇîÏîÑîÔîÖîÛîáîæîéîêîíîîîïîòîöî°ï²ï³ï"
"´ï¶ï·ï»ï¼ïÀïÆïËïÑïÒïÓïÕïÖïßïáïæïçïèïéïêïëïîïïïðïñïóïôïõïöï±ð¼ðÀðÂðÃðÄðÉðËðÐðÔðÙðÛðàðåðæðèðéðëðîðïðôð"
"öð±ñ³ñ¶ñ¹ñ»ñ¼ñÀñÄñÆñÈñËñÎñÑñÒñÔñØñÛñÜñÝñãñæñèñéñîñïñóñöñµò¼òÂòÈòÍòÔòÕòÖòÙòãòäòåòæòéòìòîòõòöò÷ò°ó²ó³ó"
"¸ó¹óºó¼ó¿óÁóÂóÈóÉóÎóÔóÕóÞóáóäóæóéóîóöó±ô²ô·ô¹ô½ôÂôÄôÅôÆôÉôÊôÏôÑôÔôÙôæôçôéôîôöô±õ²õ¸õ¹õ»õ¼õ½õ¾õ¿õÁõÂõ"
"ÃõÊõËõÌõÒõÓõÕõÖõÙõÛõßõæõçõéõìõîõôõöõ÷õ±ö²öµö¶ö¸ö¹ö»ö¼ö½ö¾ö¿öÀöÂöÃöÄöÉöËöÕöÖöØöÛöÜöÝöáöæöçöéöíöîööö°÷"
"±÷²÷³÷µ÷»÷¼÷½÷¾÷¿÷À÷Â÷Ä÷Ç÷È÷Ê÷Ð÷Ñ÷Ø÷Ù÷Ú÷Û÷Ý÷æ÷é÷ë÷î÷ð÷ñ÷ò÷ö÷²ø³ø´ø·ø¸ø¹øºø¼ø½ø¾øÀøÂøÄøÆøÇøÈøÉøËøÍøÐø"
"ÑøÒøÕøàøáøâøãøæøéøìøîøöø°ù²ù¶ù¼ù¿ùÀùÂùÃùÉùÌùÏùÐùÑùÓùÔùÕùØùÚùÜùßùæùèùéùíùîùïùðùñùöù²ú³ú¹ú»ú¼ú½ú¿úÀúÁú"
"ÂúÃúÆúÈúÊúÌúÏúÔúâúæúéúíúîúöú÷ú²û³û´û¶û¼û¾ûÁûÄûÇûÒûÛûâûæûéûîûöû²üµü¶ü¼ü¾üÁüÄüÈüÌüÐüÓüÖüÝüÞüàüâüåüæüèü"
"éüíüîüðüñüõüöü±ýµý¹ý»ý½ýÁýÃýÆýÇýÊýÌýÎýÔýÖýÜýàýáýâýåýæýèýéýîýñýòýóýöý°þµþÄþÉþÌþÎþÏþÑþÒþÓþÔþÖþÚþÛþÜþÝþ"
"àþãþäþæþéþîþñþöþ÷þ\0\0";

static char __szJF[TOTAL_CHAR_BUF]=
"ˆöµAÙE±MðÖ™”Q‚ã«Hßx“uåŽŽ¬Ên¼‹™åÝz‘»æ|îRö—K‡Lƒ¦°lð‡Åž„Å‰Åô“é°_ˆòél°YÔ]”XÁ¼„Ýwã“îMö’“úÁPÛE„¦ÇG¢”n"
" ¿ýxßb™ÑààPË|”]µ­‹‚¼‚ÝšeåP·w}ÓzöéLá”×oðTžrë]’L„Ùíf½k¸G‹ëîA–Å×CñvÊ{«M›Ñ½C˜¡ÞAäCîWöŠï–ƒ”Ó|†Îí”×Iu"
"ÇoòEë`˜ÇâoâF†ÊÂ}ß`‡[Ö{ú—Ó…Ép”xÒL‘aœ¼œÞOã|îh»[öŽŒš“ÜÄcÌŽàœûëužRŠä¼~ãUŽŸ‘ªñSÓ“à”È‡“{¼›ç|î€á‰ìnö˜ÀS"
"“ÛåVéy“áñ”‘ô¾“§Ä“ïEò}Ž›”E‡úŒWÀtøxÓàSÈ’¿U‘âšÐä@î…á‡÷B°}ˆóÄ‘Ýoý¿ƒöLºtØˆâßw™à’ßª{Å_¬“œYŒ£ƒÓ˜«J½E­a"
"‘êâãœ°`ïDìZ÷L•³Ó†¬m‡W¾ƒ”U‚zÞrºžËŽÎžÔp´uÖŽàiŠ™½IïRç„î”ö Ì@GÈAó@é_Â“å^ñžé IÞDÔnáBÉP½HšÚïSœ¡ånî‹öšõU"
"‚÷–|ín˜OŒ¢éŸÉÌJÖtÔŠëpžHÏ„×ŸÉýSÔGÈnçÜÒ‹I½fïZäˆ°aî—ìV÷IãK‘„Œm{ßB±R­ŒÆ‘B¾SÏ‰ÙÔb‹Æ½W­‹ï`çHîž÷ZµK"
"®‹Ý‹½›„Pç ïBãTò_Õlã~È”…fƒŠÔXÌIs½{™ìïjä{ïA÷XÛânÕQì–„]ïhåXÒu’¶ÚA‚ù˜¶ÔgŽSŒD½Žïlä‡°]÷V¯—„Óµ\ÊYÏž‘z"
" tãQ”‚”yÔƒ ”Þ@¿—ÇfƒzÔtÉ‰žg½‹‘ì‘¿ä†øFÅœ÷k—âCÔ’˜ªÅDi“ï„ñ¶Ø‚¥Á•Œ¤ˆ@ÂšÑbÔxÉW¹Gž®½‘ßä~øSì\®”Ø•ª„îi"
"ºŸûuÖ\¯‘“š¢•r°c‚ÎñZ·f†TšÖŠyƒ‰Ôrs¡¾c®TÝžäSødÏlÝ…“õÖvìo”¿Ì”„xž©Ã{ˆAƒ°ÕEËW¾_šåäsøc°dÏŠí^êJühƒöŸ©Ùxã^"
"‘ÑÈRÄ˜ô”ÙQœ\í˜‰¯½y¾•ãŠÖCí“‰Ñƒ«ÕC‘C¾päøÄŸýZíXØ„“ÊŽÍáuíæœ÷á×l…s¼†ÎgŒ‘†ÑˆÌ îÔŸË’‡\–‘|‹z¾y™ÂýWÍ®Œ"
"ç˜ù…ýeä^ñgÜ‡™nœÏ‰ÄçRÙ‡‘ÙÖZîl‰qùoŒÕfÓ–˜I“í¾‰±KÔ‘Ê~œZ‹¹¾i™°šè”ÌÃìç™ûRÆAý_íd¹ Æˆšg¼‰½Ë{Ÿ’®€Øš×R´TÓÈ~"
"‚òßh”ØåF‚RÔœ‡DÀã¬¾E­‘´‰äZøzýfªNÊN“vôYØœ­h”D¯d¾ššW´_×Tî^‘òßdÝšÔ–‡¿¾R™Îäuø|ýb‚äåNŽ×™Ú¼ZÙTútºY qÕ„¼š°b"
"îŠäÙ˜ÔßåŒŽ¾^ít´Xä|úƒýlóa‘vÑaØïˆ˜‹ß€”r›öüqšª˜Œ•ñðj‰º‰‹Ôòq¾Jíy ©åHøÍ˜ýríxuØ“Ùh¾Äz¸‚»@†Ü¶dÎrža"
"¾Y²GÕŠúLÌ\¾Uíw³ŒäúvÏ–ûzýp¶\‰ò“QËEƒôê@µ“›]‡IÌOñ‚ÖxøfÛx—£ÕÕŸÆcÉOéÂŽF¾låŸøŽÜOý}Œ§Ó‡Ìmä›ˆDø†¼sœÊÔ‚ŠJËG"
"‡`ðh¾~•ÒÁ`åQø ¸]üw†¾²ž‘ƒÉê‘a‰¦Ö^Ý átÔ‘ð¼ˆÕV„ê‡Òþï‚¾|åuû[üxúX‰m‹D¯ˆòœ×ŽÝvæV‘{ËN„h‡@ÜSÕIÕaðq¾ŸŸ¬ä˜"
"ùPüƒÂO¼ƒ Ù¿`‹É”ˆóHŠ½z‰T{ä\ãžœ¥è€“´ÕNÊ|‡³‡Ë˜ïƒ‹È¾˜˜qŸ˜´^ÁbåKûZÀ›Ò\ªšŸ¨¼mÓ[…ÎÔu“Œœ«Ðl‚bŽ[¾`”SÕO÷ï„"
"âðÀD™À™RŸõådù]¸M¿‡Éò±I×xœo„©”‡‘ÐäXéWªM»›ÕŒÌYˆï†‘¬‹‹¾ŒŸÍåxùOÑUëhŠWê¾bãqÀ|Õ‚Hæ@êƒœØÁdÕŽÃÍËž†hïž^¾œ"
"µZäžú’ÔLÔ“ú³C €Š×ŒBîU‚Õ††JðA¾—µaäŸùYÏ ótÒrÙ€¼ƒežEîHðˆÙ é†ŽÃ†ÝÕ˜†wðG¿P™‰åUù^Û„“ÎÌ–Ä_îw¬˜ŒÒéT…È˜ò"
"”_ˆFÂ„‡˜ßzƒžˆÖøÕ”ðNéVÆ¾‡—gáåOù‘×‡óyØ”·QŸôåƒâ}ÐMüSÓ‹¯Ÿ¿|žâcÀ@„Ý¼yá…†¡ë…Õ~ŠYðQéZ¿N™½áå›ùgºVóxÞoÉw"
"Ó›Žýš¤‘]‚ƒ†Ìï•îjåv†ƒx‘nàyqá…‡ƒfÕr†Uðléßƒ¿b—–Ÿîá•å|ùlÁuèŽïœƒSŸá·€Ó „òÆÖRÇ{‡†ðtéb‹Ü¿d˜ºá“æJù‡¹aÜV"
"ïwŽÖÀUÅfß|¼{¿˜Í‘ëE…˜ÖGÈOŽVðxéhžcÞŸ‹å¿c—n™{Ädá‘åŠù–ÒdÏ|çY¹PÔ~ëH½g‰„ßmÂ–†–õrà]Ž¤Ù|ÙYÖošëÉð}é`¿r•Ï–VâQ"
"åšù˜ô|à‡—Uî™žV“ä‚û‘ZÀwÅdéŽâ™ß\Ù~PÖ]ð~êYœO¿OÄL FâAæ}ú\ôuÕuÀ^“Æ¾Gåi±PäÂNíž CûyŸŸªqÌNìv‚ôÖ@‡“ð€é‚¿V³ˆ"
"áŸæDúBÍ¶i‘ÍÙnå‘¼o„Úç‚ŽnígÙpáŒ®YÙt¤ájÃ›œþÚIƒEÖIð‚é€‹ÔÀ_™Á•ŸÅF´“å{æXúFÑžÏu…¢ÞIÕn”‰ôï—ý“ëã•û}ß[•žƒ¯"
"ÖXÞ\£ð–ôbßŠ¿~™±ºýâOçUú_ÜE®…ÐQÕ\Â””àUÚsÝ^Œ\¸[ÕJÔAÎ‡ÀÏíƒ†ÖOŽ€ŒÀé¿zâSçIúYÒcÎ‡¼cÛ‹ôœð‹”ÀšˆÊ[”³¾„„Ž"
"ŠAž´²[Õb½{œuée…Qƒ®ÖBÆS‡zâTé“¿w™ÉâbçšûWÒM¼gô™ð‘M‡èÙM¶’Öe‰¨ëyÃ“¸C…TÖJˆ×Ær’Ðé‹¿ŠŸâæ[úpÜ]õEŽÅ‘KòGƒ¶"
"’ìéu‘©y¼xørŽrÕ›“»ô]¿‰â æŸúwÜQõG‰Î NÄœìê ÚM“]“Ïý‹˜ãÒ•”\ësçŠÙ‘Öƒ¿M‡Âé”sÀi™¾•á´ƒâkækúÏ“õRÉn…²Œ¦ŒùÝx"
"Çvñx³«CÄX¸`Ô‡ñWÅPï@Úw×•¹½‡jé’¿âjçú„÷|ÁTé]Å“°Vœ„‚îa·M’àÖiÀšJŸýÌKñ„ëUîË‡ÕT½KnÖq‡}¼¹é‘¿•â[æ“ú·d"
"‚}‡ä“ÙZ˜·Ý†›ô[Ó‘™E¬FéžÄ·NÖué˜‹ßí\™©Ã„Áïâ‚æyú–»e¼RõVœæ¼ŠúQëA‚’×VÓH«Iƒ´Ä[ÖkÌdé À`´~â^æ„ûI÷c‰žêPÙR"
"â›èDö†è¿hÝdÖ†“×‡^êHÀRâ€æ‰ú˜õTßtîD¾Vš§Åe“¸ÅRœSÒ’ðH˜s‰ÛæuÛ™„q×v†ô¾ïÙsêDž¹ÀQ—dâZè\ûXº`ÜWõq”[Üf¹à¾]"
"ƒÈòvžõðW›°ƒ|ì¶±Š×PêIÀy™´ÙSÄ’â•çSÒ@õ^ñYâgÓ^ƒr÷[Õ“›@Áw¾C„¥×S‰¿ÊrŽpêRÙBã`çMõn”¡u¸ZŠ^‚ÜÌ}—«½BÖ`Õ_‘—”€¿‚"
"„’×H™”‡‚ãÝž—ÙLâ’çN°XÒhÜUõbýXßfð^ñ{“þ‰¾„Cƒç¸D•ºÏU¿vƒL×—·Â™µ™_ÙOâŽæ °O¹~÷qŽú¾†ÅVÙUÁ_Œ‹½qÙd«FŸoÆGÙÞHàu"
"×d‰ÈÊ‰[ž]Ù—LãOçOõoôWß…î‘‘TÙVšžß‰ÔVÊ¾€Ý›ÚEÖa×™fÙDâ˜æ—°[¿‹õœ¾Ž‚È¼S·x±OîèŒ¾dœD˜ÐÃCäRÒmÅKæNÚæÊ\"
"‡ˆ‡£ñzÙWâ“æ›÷\ÙHƒÔŠZØSØž™M•þˆÔ½YœI»jÝpŽû…’ÝS¼eŽ„ñ†­^ÚBÄTãXçCõ†üNœy ZäŽìò…”MÙršäî}…Çè‚äP³Žß@‰Å“‡Æ"
"ñ€¬|—¿š{Ùc cãfç†åí÷~ŒÓŸëüc—÷V…R¹{ÑƒA”zô~‰L‘Ôž–»ìò|˜ïÙlãgçö–îC×ƒæ€ÞZÖMégÕFýgÝ”ëm‘›èüZ‰ÀŸ¦ÊV‘“§"
"óAš‘Ùgâšçh°—öž›_ÐnÕd‘Öâ¾’Ä‘Øówž³ÀC©Áx°™ñw«k˜EÙ}èpèu°’ÏXºjöˆÏx‰|ähÒŽÀLñ˜ëSO——‡Ï‡‘Yñ~šŒÙyâ‹ç…ºDÜb"
"öœÞqëŠïLÈ„¡½j‡Š½—îBàl•ƒÔ{ò”­‡˜ššÙŽãCè|Û˜õ…ÞpŒ™¯‚Îùø™ÆDœR‹ŒÔOÖVÔ„óE“¥Åüƒeò‘Ò—ýãBç’õ‰™”RšwùN‰K»hì`"
"•øÌ“òž×hŠÊ½MÊ‡O÷ñ‰š—ÓJãGè‰¯{ÜXõŒ˜Ëý”¾}Ø‚¬”Äší•ÚHšqÔ”‡uÕxØ‘ã@ê€Éœ“«óPÒ â‰çj»X¿{öa® ñTøé|œ†ÀOÕ]ƒ~"
"ëxŽX´aýRÕˆŒÏ‰]íš×gÅcá˜ˆºò‡‡ZÖoížuòU¬z˜åš›Ó]â”ç‹õ›ÞkÜPùZ¿pÜ‰¼t™zŒÃîIÎ›”f‘cí‘ø„®ŽZ‚ÉøD“åžtòSÓDèIèZ°A"
"Âgº„öN½OåeÕÖSŒ’ñRR“Ó­‚Ü›ìFÔSØiêŸœ¿žEòK™uÜÓMäDèCöO÷Mî~ÔŽ‰A½ÁR¸Fí——î¸^ÖT‰N«Eò‰Ü—ÓP²gã™èOÏ”öEŽÍÔŒ»I"
"ß_ÓžøPâ·û|ÑYòTŒO“PÀ[ÕZÔ\ÕD†îªwòsíœ™èÝMÓUìtãsèd°BÂeºûŸÜköH„e’þõŽ†áœçá„¼“p”¢ØŸƒ¼ˆs‡÷ŒÕò\—¨ÝVÄeäBèsüD"
"öK°T¾Iº™™«@“ì¶YøBØMäJ¹S„Õ¯ƒÊa“ñ‚øˆßÊwÌyž{òˆ°¸Þ_ä…æRºˆöAµñº†ÙIécÈf„tæ‚ TÐ–žož‡†òtÝT¶[äeÜgöFüt½‰”váh"
"éwÙFááƒ€µVðsûœ™ŒÕ`Éê‡”dûƒžzò~ÝWçtöTžl“½Äw„£¾oÙuÂ™ùi†¢‹ðŒÙÊ’ê–Ù\ÒCòŠ­tÝFäyõ žIÏsãtÝØ›œpå\ÓX•ç„¢ß~"
"‘‘Ðg¿s—lêŽ»n’ê²šÅL‰P‡Kò‹¬qÜ µèKÚŽöXüoÙeð’ážðI‚€LµœË]ƒH›Q›rûÃ}é}‡§ÄI¬± ‡Ú‚tˆåËCævªœò–­IÞ]±{äHõ™æ^"
"”P×‹™»Õ{“ô™‘Ö”ÔEÌ…–²mè‡Ú…ž¢˜ä¾w°W‚á·Aà—‰_ÊšóKÝUÄœãŸ°DÂœÏN÷aÀpNŽ§Ý—½oå‰ØèbßM½^„îðzæ‡šâ…^Ë_Bæi¾WÀm"
"ðBãyªbŽ®ªs‘«žTóJÝY¶UæzölÖrçPƒºÛ`Žhµ[ÐUøQÂ•ÙN‡ÌÜŽ˜Ó¶RÙ› ŽÐàwËj‡Êôé™ôÝe²Aã·„¯ŽÂ˜÷{®aäz‡ø™CÙv•x¸Qšvýˆ"
"Mã‘—‰öwØQèFäN¼™‘Q¼uÝb²€äböqýBêUërÙJ –ÒŠâxÃ@™ŽÜ|ï‹ˆ‘Ã¼qÝ`äAövîµþðDæIÜŠ‡µªŸÙd‘ÒªzÙAÌ`”tŽÎÅ‹³¼v™³"
"ÝmÃçf¯›í™ÓxömïžÕ™ß^·e a»\Ö‡Ó™òŒ”µÂ åa„žèT ÎŽ¾ªð‹ž¼w˜ Ý‚ãŒí Ï\»föc„ƒ¯BŒŽÀKŸN Þ•Ô¬Žë[×uÜˆºBà’‰|œîÌA"
"Ž½›Üž|ÀkÝyãxî@ö…ýO\0\0";

static char __szFJ[TOTAL_CHAR_BUF]=
"Ì¾Ô°ÎÛ»ñÀºÈÆÁû°ªñÚÚÌÔ¯Õâ×êï§ÇÂÀ»ñþÏÔ¾ªÇãÔ²¼î¼Ððìíù´¡Ù÷ôµ»ªÝþËÏÖüÓ®ê¤îËîû½×Ô¤ò­âÃæäöñÏÃÌ¬ÉøµþðïÖþÉÜÚÐ"
"êÛêáÁ¬ÛªîèîðÍçÂ­Ñø÷§ÄñðÌ÷úÁÝã³»úÌÌÁÔ½ÃÎÑç¤×ÛÐåËàÜöÙôÚ³Ð³Ö¤·°îçï¤ïáïí°äà¶Í¼¸¾æ®¼·Æàð÷ÍÝóæçÀÀ°¼èêìÖïêß"
"×ªîíïÌÀØãÙ¶Ùò¨¶üÙìôðÙÍÌ§èåÍÖÀÄäíáîç¨ç¶¼»¾÷Ú²·¡Ôß¼£õÎÔÉÈ§¿¥ÖèöÑöîÍÅá»ìËÊÞÏÖÇîëÍ¿ä¹óéôÇ¥×¶ÌúÎíöòð¯ðÍ¶ª"
"ä°ØºÒ¤ÂÌÑÞ¾£Ü¼Ú«ÚÉîéâÄ³ÒöÒÂÂ½öÎ«Ì¡çªÇ×ÚÜ±áÊêÕÞîöï¹ï¬ãØ¼ÊÆÄÄÙÂ¿öïÅ»æ«ÒµÀá±õÓªÏ×çöµÁ³ïç©³ñÉöÞ­Ö¾ÚÍ¼¥Âò"
"ØÍ½Î¼üïÏîìãÚÁì¶ö÷«ð×ßÃÛ¼ÇÕâ¨ç¹Â«êéÈÏÚÑ´ûÈñïÈ´³æø²¢ßõ¿é²ÒÕµ°­ÖÕÉþÔàËÕ²¬ï¿îõæíæ÷öðÙÝ³¢ÜãÇ¤¹öíÞ»æëËÙõá¥"
"·ÃêÜ³¤·ç¾¨÷¨Ýº²ÑÄâºáÂúÏÁâ¤¾¡½ÕñÀ×éÝÓ¹ÆñÐêí»äÆñ·Ñ¸Óéï½øïÛò¢±îÛî³øÌþ²Ó±·ÖÖçÆÇÌ²·Ç¾ÔÌò÷Ú¿ÌùÏú´¸ÕàïÜâÅöì"
"¾é÷áßéËï¼«äËÓæðÝ¼à°íçË¼ë°¿ÜÉÝ»Æ»ÉèÚÀÚÏêÝõ»ê¥îÎîÞïÅïÞïîöíðÁ÷þ¿­ÛõØËÑï±÷ÅÌ±ÊçÄ·£ÎÔÝªêîÚÚ³ìÛ£Ðâï£²ù¹Øæê"
"·ïð¾ØÐâú»»Å¡¾ö³Æ¿úçÙ²Ïµ®ÊúÃ³õÒîÊÇ¯ï¼âÆÃùº×Ù¶»ãÃí¸ééÀÁè½¦Â¬ÓùôÖç·çØÂîÁÙÀ³ÚÈÊ¶ºØÌàïñ¾µãÛò¡ì©ÂíöÓðµÆëÇÈ"
"á®³§ÖÀÂÙËñ·àÎ¬ÜÐÐí·íÚÛ·áêÚÔ¾ÖáÛ§îÏï±ïÚËæìªÔ¦æìð°»ÆÕ«ØÑÔ±Í¿âÐ¹ßãøÖòê±±ñË¶°ÕëáÓÕÖîÌ·Â¸éòÃ­ÖýÃÅ½¤·ëÆïöØ"
"öóßÇ·ÏÀ©¸Ëìøçºç§½ÉÕÝÂùôÁêïÁÞõÜé÷Ç¦ïÄïÎ²ûÏÕÒÃæë±«áÉ¹ãëÊÂË³Õ¿óóÆ¸ÙçÌ¸ÞÝäËßÚ½ÑèÆ×»ßõÈéð¶§Ã¾ãÅö«öÖ÷®»©°Â"
"Ñ§Å·Ñ÷ç¬ÍøÝ°Ý²êàõÙéóÉÁò¤ÏÚÍÔ÷ÞðÐì´Áëß¢ðÜí¸óêÄÔòå¾õÚ­ÚÎõéîáÂÁÇ®ïÍ÷³öõ÷­÷½ðÙ³ÝÞÆâæÔ¨ÎÍÖ¢ÀñÉ¸½á×º½¯ÞÁÀï"
"×ÊéøöÉãË³Û¶·ðÃðÏßë¶áÓìËËäµ»âíÂ´ÜÁ¸ÓïÅµ¼ÖºäîÙï¶ïìãÆö¨ì«Ñ±÷¬¶ìð¿Ùäö³Ð¥ÔÀâÞ°ÚðßÃÐÀùìòÇÏô¥ÒïÖ×´ÐÀÀÓÎîÕïÑ"
"ÒþÄÖÁÛð½ß´ÂÏËÓÇ³·¯µ»ÁýÜàÞºòý°ÀÕï³ÏÄ±ÔôàÎÔËÐ¿½õïÙö°çÖæðöàðËÎëÂ®ÂÇ»Óß£Ã»äÞñ®ñ¼ÂÚ¼öêë×¢½ëÚËõÑéöÓÊ±ÕÂ¤ãÒ"
"ðÀÇøßØ·ÜäÂçáí¿Ôî¾øç¸¼Ì¹ÛÎ½½Ï¹ýî×¹³±µÃª°÷÷²Í·¹ÝöÚðÄÛ÷ÈÅéÝ³åÑ¢È·ç²çÍÂÞ½ÅÌ¨ÈøÎÜéñ´ï¿ªÆ­ðÎöµß¼ñ¨ÇÔóÙÕÀ¸¿"
"ç×î¼ÝüÎóÌÜ¼ùéûÎ¥îÛãÊÁ¥Áéì¬ã¥Å½Ðº½ýè¨²úñ«ÂëíÃóÈÒñ²¹Ú¾ÖßÎý¼Õ°¹öêö÷ÏÀÕøÓ¸´ÂçÇî¿×°Ú¬ËÐõæéúÒ£îÐîú¼øãÈãÎöÜ"
"ö·ÇìäÉìâÌ±ôÏç±çÉ³¦ÓëÛ»ñÏêâÄÆÈòöýö×ð²µãÌüÞóèÙ¾·ñ²ÍºöÕÃàçÈØÂëÉÐËÜ×ñÉ»åÚÞÉÞÔØÑ·ïÀïï÷µð±½Äáè±ðì£»ýóÖÙáñï"
"ëð¾Ù»Ñ±öéùîò´íÏÐö´ÙÇÐ­ÄìèßÓ±óýç«¾É×¯ÍòËµ´ÚµÝîâîüÎ¤Ñ»ö¶èÅ»¶ä¯íîôÐ½ÊñìÊ´ÊõÚ®Òëêäõò¶Ûîã¼äÈÍ²µðÆÁäÉ¾ßÂ¿ù"
"ºº»·ÀéÏØñÜÒéÔ¶³ó·æïäãÉöÁò¥Æ®â¼µ³Á°ôÌçµçÓÃÕÖíÛ©ÃÌËø¾±ÅôßÔóåÂáÂçÜùÔÍîÔïêÍÇì­Î¹çåÑ¤æþÚ×õï°ìîÓïÓ÷¯Ìõ±ô·¢"
"ç»ò±ÎÀË­Ç´êãÏçÕ¢Æµì®âÇöøðÇö¸³¾¹¬·©Â÷¾ÀÐøÀ¼ÍàéüÊÊËäöüÂÍÓÇÂ£èÉµµ×ÕÓõÝ«²ÔÝ¡³åÚª¿Î´Í³®ïªö¦º«öÛ»ÁãòÎÞ¼Í¸ø"
"½ô¾¥ÚÊàë»¤¸¨´ÇÅ¥¾²öÞÈµ÷õáÛËð¼õç³·ì²øÝ¥Õ©ÉÍÇá±èîåË«ðÑöºØÙÇµè¿ËêÕÇË¸ÓÌçõæûÈÞÚÕ±ç½Ââ¾å¹öÙöúöÝÃ¹¼ÛÑÒÀ¹Ê±"
"¿öÁ¤çÊÜÑÝÛÏº³ÄÚ±ÚÇ°ùÅâÅ©³ûÏÊÍÒö¹È´Ûðá­ß±ÈÙäÓáøÔ¼Ëõ²õâÙ¸Ïîïï²ïðÔÓÁóæïØöÔòÞüäìÂ¯çôÂ¨ºìÓ§Ú¯Ç«ÏÍ³ÙÒ½¸õîó"
"ëïè¸âÈæò÷ÃÅ¸÷òÐêµº³ÜÒ¡èíÎÐ½¥äë×©æúôÇòÍÏ®ÆÀ·ÌÚÖÓþÂô×Þ½´ï·ï½ÎÙïåºÒ¼¦÷ËÂ±±¹µ·²óÀúæü×Ý¼Ô½²ÚÙ¼úÁ¾ÏÇÝöØÌ×¤"
"ÌÚöûðºßÄ¹éáïð£æýÐ÷çÐÏËÏÛ·ô¸ÇÜñÕÔê¢Ç¨Úù¸óèº¿Å·ÉæåÌåÈúðÒö¼ÒÇÉ²ß¥½ú»àÈÒÒå³æõüÚ°ÒêÐ»¶Á¸³Ó»»ÔÑ¡¾ûîþïÁÀë÷¹"
"âÉ¾Ô÷ÆÔ§ö½È£ÂÒ×±Ð¯²âÎÆÍ³ç´çÚÝñêæéþÔÇÒøîôïÖ·§ÄÑè¹÷ÅÏÌÁ©Ù¯ßÐæ´Á¯Éã¼ìäóÓüçëË¿çÏ½ºö£ê¡ÒÅ³úîøÂøæàð¶õºÏ¿Æ¾"
"ß¦éÉéâ½¬ãñÊ¨ðéíö¼ãÄÉç­ÌÐôêÐ²ÜÈÝ¤À¶×çµ÷Ò¥ï­îÍ¼ÝöùÒÚµæÛþã´¶«äþçâç½ÂÆÀÂÜ¿Ý£òÉ´¥ÖÊÇûÁÉï¥ï¸ïÇï¡ï¦ïç¹ëæã÷Ê"
"öÔð·¼ï²ÖßÕò£°¨¼êÊ¥ÂöÂÜêå¸ÆïËÃöÌââÊÉ§ÑÎö»¿ëí×óÝÅ¦ç¼çÎÒ¶Ýµ´ÊÚÆÕËÂõÍ­ï°¶îâËæææóÓãöâ¸ö¼óÛÏÔÜÀÃÄ¶íúÎÈÏß¶Ä"
"»¹îØ¸äÔ¿ãÍÚêò¦âÌæâßæÅ×ÂÎèç¾îÄåÜéÎÏÒÏÓ½×»¹õîÑÆÌ¹øãÏÑÕÀ¡ð³¸ÕßÜæ£ÔÃÌ¯Çí·è¾ºç£×ÜÚ¼ÀêéýîÖÕòÁÍÏâãÌâ½âÍÊ»Ø¨"
"ÃÇ°þÖçÑñíÓ´¿¼©¼¨Ñ¯ÚÓ±äåÇ¶ÆÉÂâ¿ð¸áóö¾ÉÊÚáµ¬½±ïùóìç¢¶ÐÎÅëÖÒèÌ¸õÄÄðï×ï©âÀÍÕÑìðÔ±ÏÚ¥Ç÷±²±ßÐÆîñïæÔÆò§Ââöç"
"öþð´ÑÇÙÏàÈåò»ë¼òÉ´µÞ¶©ÚÃÚØÂÖï¯ïâÔÄâÁæáöáÑ¼ØÇ½ÁäòçççÅ±ÁÝ¦ÒÕòÏ¸¼ÊÔÃýöÅÀµ³µµËõ§ï®ÄøÄ÷Õó¼¢ÝëðÈßàáÁÕÅÀ¿²Ð"
"»¾íÌóñÖ½¹¶ÇëÃ¨Ôþï«·¹ÈÄæñöåÁúÙ±ÆúéÄí¶¼¶°óÔµçÒÝ¯ÝÞÓ¬¹ìÂßõ¦îêïØïéæéæîÙ­ÅçÇ¿ÆÃÖÚ·×çÑò²¼ûÊ«Úº¾üåÎÏ³ïÉÖÓµç"
"Ô¸æô÷¤×¹æÁÇÞ¹¹è¬»­ç¡ç¯ñß¼Æ²÷õÏ¼­îæïëãÑòªÒû÷ÏæõÅÓÂèÇÀÇ¹éæî´íºçÁ²ïÚÁÈÃÊÍîýÂàÇýöé¶ÖÊµ·ß½àËöóïÑ¶Ú¸×¬Ö£ï³"
"ÔÏµßâÂ÷£ð¹ðÓ¹ÐÄþå¸ÄûÕùÑþðùÑâç®±àµ´Ò©´¦¹æ¹îÚÂÚ©À¾êçôõÐùîÝÕ¡öÇÑËÒõÀð÷¥ð»Éó¸ÚìÑ´¯·Ä»º²§íüÁ«ÎßÚ§Ú¹ÁÂÚß¹º"
"ê£ÁÚîÅÁåîù¾âïãÔäöè¹¨Â×´ÕÒìË°ç°ÜÊòÌÐ«±´Èüéíµ¦îÆÑÖ³ÂÀà÷ÐðÕÀöíèÐ´Ãõ¼÷éäÀ½è¶Å±µ¨ÍÉÌÖÚµÕêØÓîÉÃú¶ÍãÕÂ½Ïìæè"
"ðÅØÜÑá¿íÁ¶ðå¸ÑÃåëÚÝ§ÏôÞ´ÃÙ»°ÚþîÜïèãÔ²ö÷¢ðÂ´´ßÌâäÇ±Ó¨íÍÂ»ÁªÍÑÅ§²ÕÐéòÓÚ¦¸ÃÂÛ¸ºîÈîàï¢¸ÖïÕãÐÒ³³¥ÜÜÒ¯Âêµ±"
"´ÏÎ­Â²òîÏêÚÅ½÷²ÆÊäÛ¦¶¤îëãÓ¶¥ò©º§æçÂ³¹êÎ³çÕÏ°ÉùÊÓÚÔ¹±îÇîÚÏÎÇêËÇÀ÷ÎÊá´èÇãã¶ûËÊºÅòºÙòÑµÚ·Ñô¹®±¥âÎæööãðÉ"
"ðÖµ¯ÏÜÔæééäÜñäËççÃÖ¯êèÚÝêÞéî·øÚ÷ïßÏîò«ÊÎ÷¡Ó¥ØÉÏÅá¿ì¿ÀÅç¿ÉÉñùÁ³ò¹Ú¨ÚÄ×¸õçÕëîßï¾ï´ãÖË³Âæ÷¦ðÊðØ¶éæ©³è¹ñ"
"ÈóÖåÔúÄôÆýµýÃ¡×ÙÓËîîïµñü¹ËöÐööºè±¦éç¶ÀÖõÏ¸Á·Ö°ÆêÝ÷Æ¶Õ·îäïÊïÐÐë÷ªË§ÃÖÒäéëðüÔÁç¦¾­ó¿ÕÍÂì¼ÇÚÒ»õÔùÈíÓß¼Ø"
"Â¼ïÆïàöëáö»öç¥çÂñ÷ë÷ô¯ÜêÚ¶··ï¨Á´çï½È½¾öÏößöæÂó»®ßâÛûÍäâêÞâ¶°ä¥áýìõ½ÚÉðçÔ×È»ç¿÷¿ãÌ°ÔÞïºïÔãÇ²ü¸ëÓÅÔý·Ø"
"åýÃÆÔÎ¼ßÇ©½¢ÜÂÓ©À¯ñÍ¶ï¹áì±µöÒ¿ïÂËÌò¬±ýÑéöäÊ¦êÍÑÌÄüÁÆÁ±ç¾ñ³Ú´Ú»ÔðåÉîÌî÷ïÃï»ïÒÀ«Úíôïèý³ëÕö·¶ÌýòÃêêÑÈÉÄ"
"éõÏ½îÒïÝÁ­ã×¶Óñýöô÷©ð¼¾çÑÆ°Üé¤íªä±Áõ²ÎÆô½«ÐðÉ±À£È÷¹ôàà×¨áÎÕ»Ñ°ÕÊÒÍ¿ÇÎ°ØÛÞèÓ¿´¢½£Ç½¶ÔÜäÄöµ¼´øÂ§»Ùää¿Ñ"
"èð»À¼ÁÑå¿ÒéÖÌ²·³ë¹Ó¦½°Å¹Ù³âøÞêÆÜÌÀÖ¡ãÁä¶ì¾É¬å£ÙÐàøå°ÙÎÌ³Ù²éµÐÚèÎ´Ô½½ß¿åüµÐ³©èüÀÔÍåÐ×Ö¿éÚÂÐÁüÊýèÝ¶ÒÐ®"
"×®ÓÚÀÖ»ï¿Ùèñæµá°äÙáÕ¶ùÛëÑ¹¶ñÔÝèÈí¡ÞÒ³÷Ùðàþ¾¶²ôéÆÝÔ»½ÀÝàýèÓ½§à·ÛÛí¯Á²èâãíÇ£ÑÏÛäÞÏÄÕ±ÐèÀã¢éÍàÓé´áÝÞ»½ì"
"ÖÄâû»µ´ÓÔÖ¾¢Â¢Ûâ±ÒâüÕ¤æ¬Ê¬áâÀÌäÅÎâÂ¥²àÄÚÄÅÛÞæ¿ÕìÁ½½¿èÐÔóÓ«É¥ßùÓé×¼à¿±êÇÇÏùÖ´ìµ°ï¸´³Íí´ìÁÛÂÎ±ÂÀµ¥°Óàü"
"³Åé·ÜýÙæÌëÄÓêÊ¹µØùÀÁÞÑÊàë§Ó´×³»³Ó£ä¤ß½ÂÅÐüê¼ä«¶¯²ãÄíÑù²á¼áæÍâãÏþÎñåðµí¸É¾åÕ±Ñ«ÛÑ¼¸ÞØºø³¹ÉåÕ¶ÎÂÊ¤ÊôÁµ"
"¶¿ÀÍÖöÀ¸ëªÊÙ°®µ§½ÜÇºæÈã«²¦ãþÊÆØÄãÜÎþÛñæÖí°É¨ÂÕ¶ÏÈ¨Ø÷Âð¸§êÓ×ÇÈÈºóÃ´áÐí¨ê§ÆøÅ¨É¡ÆÓã¶±¸Â¦Õ¸ÆËÊ÷ÇâÛöæÉÞì"
"èëé¡ë²ß¸²×ÛàÃÝá«¿õÃðÎØ´ÑèïëµÌ¢ÎªæùÛ½ê¨ÕõÓ¤ÎÎÛÊ³ã¿âê¯¹Ò¼ñé­µÓäãíµÀ´âëÓµðâÀøßïÑîÜþìÇ×´Â°èãàÙìÖÉôâýÕ½ãÀ"
"È°ÔñÉ¹ÊªµòÓ¶ÔÈÒ¢¹»Ï·ÇÅ³Á±¨ÙÌ¾»ßÙÃÎ»§»÷èùÅ¢µÆµ²ÎÚìÀÂØ¶³³¡Á¹´«àðáÀâé·ãØñ¹úÊé×ÅÕ®¸Ô¹èÎ§²Þµ£¼ÃÉËÏá»¦àè¾Ç"
"ÌÎÉÕìÎá½¼ð¾Ý»áÖÍµü\0\0";
static char __szFF[TOTAL_CHAR_BUF]=
"‡@ˆ@›@«@»@À@Ã@Ì@Ò@Ö@Þ@ß@ã@ä@æ@ê@î@ï@ó@ƒAˆA‰AŠA°A²AµA·AÆAÈAÌAÔAÙAÚAÞAâAäAëAîAïAðAóAöAB‘BB¯B°BºB½BÖB"
"ÙBÚBßBáBãBäBîBïBðB÷BøBúBýB„C‘C™C C«C³C¸C½C¾CÀCÃCËCÒCÕCÖC×CâCãCäCçCèCîC‡DˆD‹DŒD”DœD°D¸DºDÀDÅDÆDÓDÕDÙD"
"ÞDäDæDèDêDîDïDðDøDüDƒE”E˜E™EžEžE«E½E¾EËEÔEÕEÙEÚEÛEÜEëEïEòEóEõEöEˆFŽF F«F¬F¸FÅFÕFÙFÝFâFåFèFìFöFøFúFG"
"G²G¸G¾GÆGÇGËGÔGÖGãGðGòGõG‚HƒHžH«H½HÓH×HÙHÚHÞHäHåHçHêHëHîHðHóHöH‡I‹I˜IœIžI I«I­I±I»I½I¾IÄIÌIÕIÖI×IÙI"
"ÚIÞIæIçIèIêIîIðI÷IûI†JŠJšJ«J¾JÌJÓJÕJÖJÙJäJæJêJóJK‡K‰K‘K±KµK½KÀKÅKÌKãKåKèKòKóKöKƒL‡L‰L’LLLÀLÄLÅLÒL"
"ÔLÙLéLïLöL÷LúL‘M”M™MMªM«M±M·M¸M½M¿MÐMÒMÓMÖMØMÙMÚMÝMßMçMîM÷M‰NNŸN NªN·N¿NÂNÊNËNÌNÏNÕNÙNäNåNæNçNðNöN"
"ùNüN‡OŒO˜OœOO°O±O½O¿OÀOÂOÈOÉOÌOÔOÕOÖOÙOÜOÞOâOãOåOçOèOöOùOýO„P‰PP“P”P±P¹P¿PÁPÅPÉPÓP×PÜPàPäPåPçPêPóP"
"øPùP…Q‘Q“Q”Q›Q·Q¸QÀQÐQÕQØQÙQÜQâQãQåQðQøQúQ‚R…RR”R™RœRžR±R¶R¼R¾RÀRÁRÅRÈRÖR×RÙRäRæRçRêRîRïRñRõRûRýRƒS"
"ŽSS”SœS¹S¼S¾SÆSÔSÖS×SØSÙSÜSÝSàSâSäSçSëSïSñSòSøSüSýS…T†T‰TT‘TžT T®T°T´TÁTÄTÕTÖT×TÙTÝTãTèTéTðTñTòTõT"
"öT†UU”U—U¶U¾U¿UÀUÏUÐUÑUÓUÙUÜUÝUãUåUçUêUëUîUòUõUŽVV–VžV°VµVºV¾V¿VÅVÊVÔVÕVÖV×VÙVÜVÝVåVæVéVìVõV÷V‡WŠW"
"ŒWšW°W½W¾WÉWËWÙWÜWÝWéWîWðWñWôWûWýWŽX”X°X´X»XÄXÏXÓXÔXÖXÜXãXäXåXæXíXöX÷XúXûXýXŠY‘YœY®Y°Y¶YºY½Y¾YÊYÌYÑY"
"ÙYÝYçYêYñYôYùYúY‡ZŠZŽZ‘ZœZ ZµZ¸Z¼ZÕZÖZÙZÞZâZäZèZéZìZïZñZ÷ZùZûZüZýZ‡[Ž[[”[°[²[µ[¶[¸[»[À[Ä[Ê[Ó[ß[â[æ["
"ë[ô[÷[û[‡\Œ\”\œ\µ\¶\»\Ê\Ì\Ï\Ò\Ô\Õ\Ö\Ù\Þ\ß\ä\å\è\ì\í\ò\÷\ú\‰]]‘]“]”]›]ž]°]¸]¾]Ë]Ó]Ô]Õ]Ö]Ü]Þ]à]é]ë]ô]"
"ù]…^‡^Š^ž^­^´^¸^½^¾^À^Ó^Ö^Ý^ß^â^ã^ä^å^æ^í^î^ð^õ^ù^‰_”_™_›_°_´_¾_À_Á_Ä_Å_Ë_Õ_Þ_ß_é_ò_ú_ý_‡`°`¸`º`¾`¿`"
"À`Á`Ì`Õ`Ö`Û`Ý`ß`ã`é`ë`ì`ï`‘aaža a­a®a°a´aµa¹aÊaÑaÕaÖaåaîaóaöa÷a‚bªb°b¾b¿bÁbÑbÔbÕbÜbÝbßbâbäbèbébôbõb"
"ýb‘cžc c°c¼c¾c¿cÄcÅcÆcÒcÙcâcécöc÷cøcücd”d—d¯d°d¶d·d¾d¿dÁdÄdÅdÌdÒdÕd×dÙdÝdßdådèdídødƒeƒe„eše·e»e¼eÂe"
"ÄeÅeÖeÙeÝeäeåeéeýeƒf…f”f™f·f»f½fÅfÇfÈfÕfÜfßfãfçfíføfýf—gšgžg²g¼g½gÂgÎgÐgÔg×gÙgÜgâgãgégígñgùgýg„h†hŽh"
"h­h»h¿hÒh×hßháhähçhéhëhîhïhðhühi¶i¾iÀiÖiØiàiåiæiîiùi‡jºj»j½jËjájâjçjîjïjðj«k½kÀkÖkÜkÞkâkæk÷k—lžl°l"
"¾lÏlÐlÕl×lÙlàlélîlïlðlölùlýl‰mŒm¬m²m¼mÀmÌmÒmÝmßmëmömŽn‘n”n—n™nn»nÈnÉnÊnÐnÔnÕnÙnânånìnínõnœožoŸo¼o½o"
"¾oÇoÖoÖo×oÝoÞoâoìoõoùoüoŽp“pœp¾p¿pÀpÉpÔpÙpÝpÞpèpëpúpýp„q‰q˜qšqq qªq¬q¼q½qÖqÞqãqðqòqõqöq÷qüqƒrŽr”r•r"
"›ržr¿rÆrÊrÎrÒrÔrÕrÖrÙrÞrërõrørýr…sˆsss˜ssªs¼s¿sÏsÙsÚsãsäsèsësðsòs‚t„t”tžt t­tºt¼tÀtÔtÖtÙtßtátãtçt"
"ìtítðtòtótútüt‡uuu“u™uœuužu´u¼uÁuÏuÒuÔuÕuÖu×uÙuàuáuäuåuæuèuéuëuôuûu‘v“v”všv¼v¿vÇvÖv×vÙvÝvåvævìvñv"
"òvövúv†wšwªw·w¼w¾w¿wÀwÁwÄwÉwÊwÚwÝwßwàwéwíwîwïwñwówöwúwüwƒx„x”x•x·x¼xÁxÏxÓxÔxÕxÖx×xÙxÛxÝxßxâxãxåxëxíx"
"ðxñxóxøxüxýxyŠy”yœy¼y½y¾yÀyÌyÙyÝyàyãyäyæyéyëyíyóyûy‚zƒz‡z‹z‘z”z™zžzªz¬z½z¿zÄzÓzÝzßzäzæzðzñzøzûz{‘{"
"“{™{š{{ž{ª{¯{±{¹{¼{½{½{¿{Ã{Ç{Ê{Ë{Ô{Õ{Ö{ä{å{ñ{÷{ƒ|‰|‰|‘|–|ž|¬|¾|¿|À|Ê|Ë|Ï|Ó|Ù|Ü|ß|ã|ä|å|æ|ç|è|é|ò|ô|"
"÷|ø|û|‚}‡}}°}¾}Â}Ã}Ì}Ù}â}æ}é}î}ð}ò}û}ý}ƒ~´~¹~¼~¾~¿~È~Ê~Ô~Õ~Ù~ß~ã~ä~î~ð~ñ~ò~ô~÷~‚€ƒ€Ž€”€ €®€²€·€¾€Ù€"
"ß€â€æ€è€é€ê€î€ð€ñ€‡’”˜½ÄÊÎÏÔÕÝâäåéîðø„‚‡‚‹‚‚”‚­‚¯‚¸‚¼‚¿‚Ô‚Ø‚Ý‚â‚æ‚ç‚è‚é‚ï‚ð‚ñ‚ƒ"
"‚ƒ„ƒ•ƒ¯ƒ´ƒ¼ƒ¾ƒ¿ƒÔƒÖƒ×ƒßƒåƒêƒïƒúƒûƒüƒ‰„Ž„‘„ª„·„º„¼„¾„Â„Ã„Ô„Õ„Û„á„æ„ç„ï„ñ„ø„ú„®…Ó…Ú…Ý…ß…á…ä…ç…ë…î…ò…õ…"
"ö…ù…†ƒ†‡††œ†º†¼†¾†Ó†Õ†Ö†Ý†ä†ç†é†ï†ñ†õ†ø†…‡”‡ž‡­‡¾‡¿‡È‡Ë‡Î‡Ó‡Ô‡Ö‡×‡Ù‡Ü‡à‡á‡ä‡æ‡è‡ê‡ð‡ò‡ù‡‡ˆˆˆ”ˆšˆ"
"¯ˆ³ˆºˆ¼ˆÆˆÕˆØˆÜˆäˆïˆðˆòˆöˆýˆƒ‰—‰™‰´‰¼‰½‰¾‰¿‰É‰Ê‰Ï‰Ü‰ß‰á‰â‰æ‰è‰ñ‰ò‰ƒŠ‡ŠŠŠ±Š¼Š¿ŠÏŠÒŠÔŠÕŠÜŠßŠãŠåŠçŠëŠ"
"îŠòŠöŠ‰‹‹‹Œ‹˜‹­‹®‹¼‹½‹¿‹Ó‹×‹Û‹Ý‹â‹ç‹é‹î‹ï‹ð‹ò‹ý‹‹Œ“Œ˜ŒšŒ®Œ³Œ¾ŒÔŒÕŒ×ŒáŒãŒèŒòŒõŒ‡Œ‘¬ºÓÔÙàä"
"íîïöøú„ŽŒŽŒŽ™Ž Ž¬Ž¯Ž³Ž½Ž¾ŽÊŽËŽÌŽÒŽÔŽÕŽÖŽ×ŽÙŽÚŽÜŽâŽåŽèŽéŽêŽõŽöŽøŽŒŸ¯¼¾ÀÃÉÊÓÔÕ×Ù"
"Ýàáâãäçèõý‚œ®¶½ÉÍÏØÙÜàáéêîðúûýŒ‘‘‘™‘š‘ž‘­‘¯‘Ä‘Í‘Ó‘Ô‘Ø‘Ù‘á‘ã‘å‘é‘ê‘í‘ò‘"
"ù‘„’…’Œ’Ÿ’°’¶’¾’Ä’È’Ê’Ë’Ò’Ô’à’â’ç’é’ð’ö’ú’„“‡“‘““¬“´“µ“Â“Ã“Ä“Å“Ì“Ï“Ó“Ô“Õ“Ø“á“â“ã“ä“æ“é“í“ƒ”™” ”¬”®”"
"Â”È”Ì”Ï”Ô”Õ”Ö”Ø”Ý”à”á”â”é”í”î”ñ”ò”ô”ý”¾•¿•Á•Â•Ò•×•Ø•á•â•ã•í•ï•…–†––—–ž– –Â–Ì–Ï–Ð–Ó–Ô–ê–ì–ï–ð–ò–ö–ù–"
"ú–—‘———š—ž—°—½—¾—¿—Ò—×—Ù—Ü—Ý—à—æ—í—î—ï—ö—ú—…˜‡˜˜Ÿ˜¬˜¾˜¿˜Â˜Ä˜Í˜Ó˜Õ˜Ù˜Û˜á˜â˜ä˜ç˜é˜í˜ñ˜ö˜ù˜ú˜‰™Š™Œ™™™"
"™°™¼™Â™Ó™Õ™Ö™Û™â™ã™ç™í™î™ô™õ™ø™Œšššªš²š¼š¾šÂšÄšÊšØšÝšâšåšçšíšöšŽ››‘›š›¯›»›¼›½›À›Ã›Î›Ó›Õ›Ø›Ù›Ü›Ý›â›"
"ä›å›æ›õ›ªœµœ¼œ¾œÂœÄœÅœÉœÔœØœãœæœíœïœòœôœõœöœûœ„‡ˆ“—œªµ¹¼¿ÆÈÌÑØÙäçéîøƒž„ž‰ž"
"‹žž•žšžºžÅžËžÎžÏžÑžÓžØžÝžážãžäžížîžïžòžöžŽŸ•ŸŸŸªŸ¯ŸºŸ¾ŸÄŸÔŸÕŸØŸÞŸáŸãŸäŸåŸæŸéŸêŸûŸ˜ ® ± ¹ Â Ï Ò Ó Ù "
"Ü Ý â æ ç é ê í õ ö ø „¡†¡”¡˜¡œ¡¡„¢…¢†¢Œ¢”¢š¢¢ž¢„£‡£Œ££—£Œ¤Ž¤¤š¤‚¥„¥“¥œ¥ƒ¦„¦‰¦Œ¦Ÿ¦‡§Œ§Ž§“§š§§‰¨"
"—¨Ÿ¨„©©‘©™©ž©Ÿ© ©‘ª˜ªšªƒ«‘«“«—«œ«Ž¬‘¬¬Ÿ¬­µ­ƒ®Ž®ž®ƒ¯‰¯ƒ°™°›°™±…²²‡³‹³”³•³™³³ž³ƒ´“´™´ž´‡µ”µ™µƒ¶’¶"
"˜¶ì¶˜·â·“¸°¸‹¹¹ž¹¼¹ƒºˆº‰ºº•º˜º‘»“»™»ƒ¼Ž½½“½™½¹½†¾‰¾Ž¾™¾¾‡¿‰¿‘¿”¿—¿œ¿ ¿‡À‰ÀŒÀÀ”À™ÀÁ™Á‡Â™Â·ÂéÂŒÃ"
"ŽÃ‘Ã‰ÄÄžÄ„Å‰Å‰ÅŽÅÅ–Å‹ÆŒÆÆ“ÆÆ…Ç˜Ç‚ÈƒÈ…È‰È‹È‚ÉƒÉ‹É™ÉÉŸÉ†Ê‡ÊŠÊœÊ‡Ë˜Ë†Ì‡ÌˆÌ”ÌŽÍÍ‘ÍÍŸÍÃÍ‚Î…Î†Î‰ÎŽÎ"
"“Î™Î Î‡ÏŒÏ“Ï•ÏœÏÐ‘Ð’Ð˜ÐšÐ†Ñ‰Ñ‘Ñ™Ñ›Ñ‡ÒŒÒ‘Ò•ÒÒ„ÓŒÓ“Ó˜ÓƒÔˆÔ‹Ô‘Ô•Ô„ÕŒÕÕŽÖ‘ÖšÖ„×ˆ×Ž×“×‰ØØ‘Ø”ØœØ„ÙŒÙ‘Ù"
" Ù„Ú‡Ú™ÚšÚ‰ÛÛ“Û‚Ü†Ü‹ÜÜ“Ü›Ü„Ý†ÝãÝ Þˆß‹ß‘ß’ß’à”à™à‚á†á“á•ááŸááá÷áââ‘âšââ‚ã˜ãÀã‚äŠää“ä˜äšäˆå‹å“å"
"˜å™åšåßåœæÚæƒçç•çœç†è‡è™èšè“éžéôé„ê‘ê’ê‹ë“ëšëŸëŽì‘ì’ì“ì™ìœì»ìÃìíí“íåí„î†î—îœîŸî î“ï˜ï¾ïÁï‹ðð‘ðâð"
"„ñ“ñ•ññµñ‚ò„òˆò‰ò‘ò˜òÉòˆó‚ôƒô†ô‰ô‘ô“ô™ôôŸô“õžõŸõöƒöˆö›ö‚÷‡÷÷÷—÷‚ø‡ø•øÖø‚ùŒùÎù‡úŽú“úú‚ûŽûœûÅüŽý"
"ýŸýºýþ’þ“þ•þœþµþ\0\0";

static void code_to_file(const char *filename, const char *name_pre,
	const char *from_tab, const char *to_tab)
{
	char  buf[255];
	ACL_FILE *fp;
	int   i;
	unsigned short table[65535];

	// ³õÊ¼»¯
	for (i = 0; i < 65535; i++) {
		table[i] = 0xffff;
	}
	unsigned short *ptr1 = (unsigned short*) from_tab;
	unsigned short *ptr2 = (unsigned short*) to_tab;
	while (*ptr1 != 0) {
		table[*ptr1] = *ptr2;
		if (*ptr2 == 0xa1b3) {
			printf(">>>>>ptr2: 0x%x, %d, ptr1: %d, 0x%x\n", *ptr2, *ptr2, *ptr1, *ptr1);
		}
		*ptr1++;
		*ptr2++;
	}

	printf(">>>>>table[41395]: 0x%x\n", table[41395]);

	fp = acl_fopen(filename, "w+");
	snprintf(buf, sizeof(buf), "static unsigned short %s[] = {\n", name_pre);
	acl_fwrite(buf, strlen(buf), 1, fp);
	for (i = 0; i < 65535; i++) {
		if (i % 8 == 0) {
			if (i == 0)
				snprintf(buf, sizeof(buf), "\t");
			else
				snprintf(buf, sizeof(buf), "\n\t");
			acl_fwrite(buf, strlen(buf), 1, fp);
		}
		snprintf(buf, sizeof(buf), "0x%x,", table[i]);
		acl_fwrite(buf, strlen(buf), 1, fp);
	}
	snprintf(buf, sizeof(buf), "\n};\n");
	acl_fwrite(buf, strlen(buf), 1, fp);
	acl_fclose(fp);
}

static void code_map(const char *filename, const char *data)
{
	ACL_FILE *fp = acl_fopen(filename, "w+");
	unsigned short *ptr = (unsigned short*) data;
	char  buf[16];

	acl_fputs("static unsigned short __map_tab[] = {", fp);
	while (*ptr != 0) {
		snprintf(buf, sizeof(buf), "0x%x,", *ptr);
		acl_fwrite(buf, strlen(buf), 1, fp);
		ptr++;
	}
	acl_fputs("\n};", fp);
	acl_fclose(fp);
}

static void write_code(void)
{
	printf("jt2ft\n");
	code_to_file("jt2ft.h", "__jt2ft_tab", __szJJ, __szJF);
	printf("ft2jt\n");
	code_to_file("ft2jt.h", "__ft2jt_tab", __szFF, __szFJ);
	getchar();

	return;
	//³¡´¡·¡¾¡À¡Ã¡Å¡É¡
	code_map("j2f.txt", "³¡´¡·¡¾¡À¡Ã¡Å¡É¡\0\0");
	char  buf[256];
	unsigned short a[] = {0xf688,0x3666,0x3636,0x2c,0x81f0,0x99d6,0x5194,0xe382, 0};
	unsigned short *p = (unsigned short*) buf, i;

	memset(buf, 0, sizeof(buf));

	for (i = 0; a[i]; i++) {
		*p++ = a[i];
	}

	printf(">>%S\n", buf);
	getchar();
}

int main(int argc, char *argv[])
{
	write_code();
	return (0);
}
