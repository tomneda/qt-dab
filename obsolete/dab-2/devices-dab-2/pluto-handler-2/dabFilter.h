
#
//
//	To avoid including the ad9361 library (just because in
//	Ubuntu 16.04, used for generating an appImage, only
//	an old version is available, the parameters of a filter
//	
//	Fpass     = 1536000 / 2;
//	Fstop     = Fpass * 1.2;
//	wnomTX    = 1.6 * Fstop;  // dummy here
//	wnomRX    = 1536000; // RF bandwidth of analog filter
//
//	were extracted and the filter configuration is loaded

//"BWRX 1536000\n"
const char * dabFilter =
"RX 3 GAIN -12 DEC 4\n"
"-250\n"
"296\n"
"-324\n"
"458\n"
"100\n"
"766\n"
"652\n"
"1100\n"
"936\n"
"1020\n"
"618\n"
"366\n"
"-146\n"
"-406\n"
"-668\n"
"-578\n"
"-398\n"
"26\n"
"374\n"
"688\n"
"704\n"
"530\n"
"96\n"
"-360\n"
"-766\n"
"-884\n"
"-724\n"
"-248\n"
"334\n"
"876\n"
"1118\n"
"990\n"
"458\n"
"-278\n"
"-1006\n"
"-1414\n"
"-1352\n"
"-758\n"
"172\n"
"1152\n"
"1798\n"
"1850\n"
"1198\n"
"24\n"
"-1312\n"
"-2312\n"
"-2572\n"
"-1880\n"
"-380\n"
"1490\n"
"3068\n"
"3724\n"
"3038\n"
"1044\n"
"-1768\n"
"-4490\n"
"-6068\n"
"-5578\n"
"-2550\n"
"2850\n"
"9794\n"
"16962\n"
"22850\n"
"26166\n"
"26166\n"
"22850\n"
"16962\n"
"9794\n"
"2850\n"
"-2550\n"
"-5578\n"
"-6068\n"
"-4490\n"
"-1768\n"
"1044\n"
"3038\n"
"3724\n"
"3068\n"
"1490\n"
"-380\n"
"-1880\n"
"-2572\n"
"-2312\n"
"-1312\n"
"24\n"
"1198\n"
"1850\n"
"1798\n"
"1152\n"
"172\n"
"-758\n"
"-1352\n"
"-1414\n"
"-1006\n"
"-278\n"
"458\n"
"990\n"
"1118\n"
"876\n"
"334\n"
"-248\n"
"-724\n"
"-884\n"
"-766\n"
"-360\n"
"96\n"
"530\n"
"704\n"
"688\n"
"374\n"
"26\n"
"-398\n"
"-578\n"
"-668\n"
"-406\n"
"-146\n"
"366\n"
"618\n"
"1020\n"
"936\n"
"1100\n"
"652\n"
"766\n"
"100\n"
"458\n"
"-324\n"
"296\n"
"-250\n";
