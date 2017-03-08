//
// Created by QY on 2016-12-20.
//

#ifndef UNTITLED3_REFLECTION_HPP
#define UNTITLED3_REFLECTION_HPP


#include <iostream>
#include <sstream>
#include <string>
#include <tuple>
#include <iomanip>
#include <map>
#include <vector>
#include <array>
#include <type_traits>

/******************************************/
/* arg list expand macro, now support 120 args */
#define MAKE_ARG_LIST_1(op, arg, ...)   op(arg)
#define MAKE_ARG_LIST_2(op, arg, ...)   op(arg), MARCO_EXPAND(MAKE_ARG_LIST_1(op, __VA_ARGS__))
#define MAKE_ARG_LIST_3(op, arg, ...)   op(arg), MARCO_EXPAND(MAKE_ARG_LIST_2(op, __VA_ARGS__))
#define MAKE_ARG_LIST_4(op, arg, ...)   op(arg), MARCO_EXPAND(MAKE_ARG_LIST_3(op, __VA_ARGS__))
#define MAKE_ARG_LIST_5(op, arg, ...)   op(arg), MARCO_EXPAND(MAKE_ARG_LIST_4(op, __VA_ARGS__))
#define MAKE_ARG_LIST_6(op, arg, ...)   op(arg), MARCO_EXPAND(MAKE_ARG_LIST_5(op, __VA_ARGS__))
#define MAKE_ARG_LIST_7(op, arg, ...)   op(arg), MARCO_EXPAND(MAKE_ARG_LIST_6(op, __VA_ARGS__))
#define MAKE_ARG_LIST_8(op, arg, ...)   op(arg), MARCO_EXPAND(MAKE_ARG_LIST_7(op, __VA_ARGS__))
#define MAKE_ARG_LIST_9(op, arg, ...)   op(arg), MARCO_EXPAND(MAKE_ARG_LIST_8(op, __VA_ARGS__))
#define MAKE_ARG_LIST_10(op, arg, ...)  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_9(op, __VA_ARGS__))
#define MAKE_ARG_LIST_11(op, arg, ...)  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_10(op, __VA_ARGS__))
#define MAKE_ARG_LIST_12(op, arg, ...)  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_11(op, __VA_ARGS__))
#define MAKE_ARG_LIST_13(op, arg, ...)  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_12(op, __VA_ARGS__))
#define MAKE_ARG_LIST_14(op, arg, ...)  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_13(op, __VA_ARGS__))
#define MAKE_ARG_LIST_15(op, arg, ...)  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_14(op, __VA_ARGS__))
#define MAKE_ARG_LIST_16(op, arg, ...)  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_15(op, __VA_ARGS__))
#define MAKE_ARG_LIST_17(op, arg, ...)  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_16(op, __VA_ARGS__))
#define MAKE_ARG_LIST_18(op, arg, ...)  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_17(op, __VA_ARGS__))
#define MAKE_ARG_LIST_19(op, arg, ...)  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_18(op, __VA_ARGS__))
#define MAKE_ARG_LIST_20(op, arg, ...)  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_19(op, __VA_ARGS__))
#define MAKE_ARG_LIST_21(op, arg, ...)  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_20(op, __VA_ARGS__))
#define MAKE_ARG_LIST_22(op, arg, ...)  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_21(op, __VA_ARGS__))
#define MAKE_ARG_LIST_23(op, arg, ...)  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_22(op, __VA_ARGS__))
#define MAKE_ARG_LIST_24(op, arg, ...)  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_23(op, __VA_ARGS__))
#define MAKE_ARG_LIST_25(op, arg, ...)  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_24(op, __VA_ARGS__))
#define MAKE_ARG_LIST_26(op, arg, ...)  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_25(op, __VA_ARGS__))
#define MAKE_ARG_LIST_27(op, arg, ...)  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_26(op, __VA_ARGS__))
#define MAKE_ARG_LIST_28(op, arg, ...)  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_27(op, __VA_ARGS__))
#define MAKE_ARG_LIST_29(op, arg, ...)  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_28(op, __VA_ARGS__))
#define MAKE_ARG_LIST_30(op, arg, ...)  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_29(op, __VA_ARGS__))
#define MAKE_ARG_LIST_31(op, arg, ...)  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_30(op, __VA_ARGS__))
#define MAKE_ARG_LIST_32(op, arg, ...)  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_31(op, __VA_ARGS__))
#define MAKE_ARG_LIST_33(op, arg, ...)  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_32(op, __VA_ARGS__))
#define MAKE_ARG_LIST_34(op, arg, ...)  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_33(op, __VA_ARGS__))
#define MAKE_ARG_LIST_35(op, arg, ...)  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_34(op, __VA_ARGS__))
#define MAKE_ARG_LIST_36(op, arg, ...)  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_35(op, __VA_ARGS__))
#define MAKE_ARG_LIST_37(op, arg, ...)  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_36(op, __VA_ARGS__))
#define MAKE_ARG_LIST_38(op, arg, ...)  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_37(op, __VA_ARGS__))
#define MAKE_ARG_LIST_39(op, arg, ...)  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_38(op, __VA_ARGS__))
#define MAKE_ARG_LIST_40(op, arg, ...)  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_39(op, __VA_ARGS__))
#define MAKE_ARG_LIST_41(op, arg, ...)  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_40(op, __VA_ARGS__))
#define MAKE_ARG_LIST_42(op, arg, ...)  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_41(op, __VA_ARGS__))
#define MAKE_ARG_LIST_43(op, arg, ...)  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_42(op, __VA_ARGS__))
#define MAKE_ARG_LIST_44(op, arg, ...)  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_43(op, __VA_ARGS__))
#define MAKE_ARG_LIST_45(op, arg, ...)  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_44(op, __VA_ARGS__))
#define MAKE_ARG_LIST_46(op, arg, ...)  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_45(op, __VA_ARGS__))
#define MAKE_ARG_LIST_47(op, arg, ...)  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_46(op, __VA_ARGS__))
#define MAKE_ARG_LIST_48(op, arg, ...)  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_47(op, __VA_ARGS__))
#define MAKE_ARG_LIST_49(op, arg, ...)  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_48(op, __VA_ARGS__))
#define MAKE_ARG_LIST_50(op, arg, ...)  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_49(op, __VA_ARGS__))
#define MAKE_ARG_LIST_51(op, arg, ...)  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_50(op, __VA_ARGS__))
#define MAKE_ARG_LIST_52(op, arg, ...)  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_51(op, __VA_ARGS__))
#define MAKE_ARG_LIST_53(op, arg, ...)  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_52(op, __VA_ARGS__))
#define MAKE_ARG_LIST_54(op, arg, ...)  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_53(op, __VA_ARGS__))
#define MAKE_ARG_LIST_55(op, arg, ...)  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_54(op, __VA_ARGS__))
#define MAKE_ARG_LIST_56(op, arg, ...)  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_55(op, __VA_ARGS__))
#define MAKE_ARG_LIST_57(op, arg, ...)  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_56(op, __VA_ARGS__))
#define MAKE_ARG_LIST_58(op, arg, ...)  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_57(op, __VA_ARGS__))
#define MAKE_ARG_LIST_59(op, arg, ...)  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_58(op, __VA_ARGS__))
#define MAKE_ARG_LIST_60(op, arg, ...)  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_59(op, __VA_ARGS__))
#define MAKE_ARG_LIST_61(op, arg, ...)  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_60(op, __VA_ARGS__))
#define MAKE_ARG_LIST_62(op, arg, ...)  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_61(op, __VA_ARGS__))
#define MAKE_ARG_LIST_63(op, arg, ...)  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_62(op, __VA_ARGS__))
#define MAKE_ARG_LIST_64(op, arg, ...)  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_63(op, __VA_ARGS__))
#define MAKE_ARG_LIST_65(op, arg, ...)  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_64(op, __VA_ARGS__))
#define MAKE_ARG_LIST_66(op, arg, ...)  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_65(op, __VA_ARGS__))
#define MAKE_ARG_LIST_67(op, arg, ...)  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_66(op, __VA_ARGS__))
#define MAKE_ARG_LIST_68(op, arg, ...)  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_67(op, __VA_ARGS__))
#define MAKE_ARG_LIST_69(op, arg, ...)  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_68(op, __VA_ARGS__))
#define MAKE_ARG_LIST_70(op, arg, ...)  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_69(op, __VA_ARGS__))
#define MAKE_ARG_LIST_71(op, arg, ...)  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_70(op, __VA_ARGS__))
#define MAKE_ARG_LIST_72(op, arg, ...)  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_71(op, __VA_ARGS__))
#define MAKE_ARG_LIST_73(op, arg, ...)  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_72(op, __VA_ARGS__))
#define MAKE_ARG_LIST_74(op, arg, ...)  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_73(op, __VA_ARGS__))
#define MAKE_ARG_LIST_75(op, arg, ...)  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_74(op, __VA_ARGS__))
#define MAKE_ARG_LIST_76(op, arg, ...)  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_75(op, __VA_ARGS__))
#define MAKE_ARG_LIST_77(op, arg, ...)  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_76(op, __VA_ARGS__))
#define MAKE_ARG_LIST_78(op, arg, ...)  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_77(op, __VA_ARGS__))
#define MAKE_ARG_LIST_79(op, arg, ...)  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_78(op, __VA_ARGS__))
#define MAKE_ARG_LIST_80(op, arg, ...)  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_79(op, __VA_ARGS__))
#define MAKE_ARG_LIST_81(op, arg, ...)  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_80(op, __VA_ARGS__))
#define MAKE_ARG_LIST_82(op, arg, ...)  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_81(op, __VA_ARGS__))
#define MAKE_ARG_LIST_83(op, arg, ...)  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_82(op, __VA_ARGS__))
#define MAKE_ARG_LIST_84(op, arg, ...)  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_83(op, __VA_ARGS__))
#define MAKE_ARG_LIST_85(op, arg, ...)  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_84(op, __VA_ARGS__))
#define MAKE_ARG_LIST_86(op, arg, ...)  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_85(op, __VA_ARGS__))
#define MAKE_ARG_LIST_87(op, arg, ...)  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_86(op, __VA_ARGS__))
#define MAKE_ARG_LIST_88(op, arg, ...)  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_87(op, __VA_ARGS__))
#define MAKE_ARG_LIST_89(op, arg, ...)  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_88(op, __VA_ARGS__))
#define MAKE_ARG_LIST_90(op, arg, ...)  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_89(op, __VA_ARGS__))
#define MAKE_ARG_LIST_91(op, arg, ...)  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_90(op, __VA_ARGS__))
#define MAKE_ARG_LIST_92(op, arg, ...)  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_91(op, __VA_ARGS__))
#define MAKE_ARG_LIST_93(op, arg, ...)  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_92(op, __VA_ARGS__))
#define MAKE_ARG_LIST_94(op, arg, ...)  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_93(op, __VA_ARGS__))
#define MAKE_ARG_LIST_95(op, arg, ...)  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_94(op, __VA_ARGS__))
#define MAKE_ARG_LIST_96(op, arg, ...)  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_95(op, __VA_ARGS__))
#define MAKE_ARG_LIST_97(op, arg, ...)  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_96(op, __VA_ARGS__))
#define MAKE_ARG_LIST_98(op, arg, ...)  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_97(op, __VA_ARGS__))
#define MAKE_ARG_LIST_99(op, arg, ...)  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_98(op, __VA_ARGS__))
#define MAKE_ARG_LIST_100(op, arg, ...)  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_99(op, __VA_ARGS__))
#define MAKE_ARG_LIST_101(op, arg, ...)  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_100(op, __VA_ARGS__))
#define MAKE_ARG_LIST_102(op, arg, ...)  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_101(op, __VA_ARGS__))
#define MAKE_ARG_LIST_103(op, arg, ...)  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_102(op, __VA_ARGS__))
#define MAKE_ARG_LIST_104(op, arg, ...)  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_103(op, __VA_ARGS__))
#define MAKE_ARG_LIST_105(op, arg, ...)  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_104(op, __VA_ARGS__))
#define MAKE_ARG_LIST_106(op, arg, ...)  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_105(op, __VA_ARGS__))
#define MAKE_ARG_LIST_107(op, arg, ...)  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_106(op, __VA_ARGS__))
#define MAKE_ARG_LIST_108(op, arg, ...)  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_107(op, __VA_ARGS__))
#define MAKE_ARG_LIST_109(op, arg, ...)  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_108(op, __VA_ARGS__))
#define MAKE_ARG_LIST_110(op, arg, ...)  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_109(op, __VA_ARGS__))
#define MAKE_ARG_LIST_111(op, arg, ...)  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_110(op, __VA_ARGS__))
#define MAKE_ARG_LIST_112(op, arg, ...)  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_111(op, __VA_ARGS__))
#define MAKE_ARG_LIST_113(op, arg, ...)  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_112(op, __VA_ARGS__))
#define MAKE_ARG_LIST_114(op, arg, ...)  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_113(op, __VA_ARGS__))
#define MAKE_ARG_LIST_115(op, arg, ...)  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_114(op, __VA_ARGS__))
#define MAKE_ARG_LIST_116(op, arg, ...)  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_115(op, __VA_ARGS__))
#define MAKE_ARG_LIST_117(op, arg, ...)  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_116(op, __VA_ARGS__))
#define MAKE_ARG_LIST_118(op, arg, ...)  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_117(op, __VA_ARGS__))
#define MAKE_ARG_LIST_119(op, arg, ...)  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_118(op, __VA_ARGS__))
#define MAKE_ARG_LIST_120(op, arg, ...)  op(arg), MARCO_EXPAND(MAKE_ARG_LIST_119(op, __VA_ARGS__))

#define SEPERATOR ,
#define CON_STR_1(element, ...) #element
#define CON_STR_2(element, ...) #element SEPERATOR MARCO_EXPAND(CON_STR_1(__VA_ARGS__))
#define CON_STR_3(element, ...) #element SEPERATOR MARCO_EXPAND(CON_STR_2(__VA_ARGS__))
#define CON_STR_4(element, ...) #element SEPERATOR MARCO_EXPAND(CON_STR_3(__VA_ARGS__))
#define CON_STR_5(element, ...) #element SEPERATOR MARCO_EXPAND(CON_STR_4(__VA_ARGS__))
#define CON_STR_6(element, ...) #element SEPERATOR MARCO_EXPAND(CON_STR_5(__VA_ARGS__))
#define CON_STR_7(element, ...) #element SEPERATOR MARCO_EXPAND(CON_STR_6(__VA_ARGS__))
#define CON_STR_8(element, ...) #element SEPERATOR MARCO_EXPAND(CON_STR_7(__VA_ARGS__))
#define CON_STR_9(element, ...) #element SEPERATOR MARCO_EXPAND(CON_STR_8(__VA_ARGS__))
#define CON_STR_10(element, ...) #element SEPERATOR MARCO_EXPAND(CON_STR_9(__VA_ARGS__))
#define CON_STR_11(element, ...) #element SEPERATOR MARCO_EXPAND(CON_STR_10(__VA_ARGS__))
#define CON_STR_12(element, ...) #element SEPERATOR MARCO_EXPAND(CON_STR_11(__VA_ARGS__))
#define CON_STR_13(element, ...) #element SEPERATOR MARCO_EXPAND(CON_STR_12(__VA_ARGS__))
#define CON_STR_14(element, ...) #element SEPERATOR MARCO_EXPAND(CON_STR_13(__VA_ARGS__))
#define CON_STR_15(element, ...) #element SEPERATOR MARCO_EXPAND(CON_STR_14(__VA_ARGS__))
#define CON_STR_16(element, ...) #element SEPERATOR MARCO_EXPAND(CON_STR_15(__VA_ARGS__))
#define CON_STR_17(element, ...) #element SEPERATOR MARCO_EXPAND(CON_STR_16(__VA_ARGS__))
#define CON_STR_18(element, ...) #element SEPERATOR MARCO_EXPAND(CON_STR_17(__VA_ARGS__))
#define CON_STR_19(element, ...) #element SEPERATOR MARCO_EXPAND(CON_STR_18(__VA_ARGS__))
#define CON_STR_20(element, ...) #element SEPERATOR MARCO_EXPAND(CON_STR_19(__VA_ARGS__))
#define CON_STR_21(element, ...) #element SEPERATOR MARCO_EXPAND(CON_STR_20(__VA_ARGS__))
#define CON_STR_22(element, ...) #element SEPERATOR MARCO_EXPAND(CON_STR_21(__VA_ARGS__))
#define CON_STR_23(element, ...) #element SEPERATOR MARCO_EXPAND(CON_STR_22(__VA_ARGS__))
#define CON_STR_24(element, ...) #element SEPERATOR MARCO_EXPAND(CON_STR_23(__VA_ARGS__))
#define CON_STR_25(element, ...) #element SEPERATOR MARCO_EXPAND(CON_STR_24(__VA_ARGS__))
#define CON_STR_26(element, ...) #element SEPERATOR MARCO_EXPAND(CON_STR_25(__VA_ARGS__))
#define CON_STR_27(element, ...) #element SEPERATOR MARCO_EXPAND(CON_STR_26(__VA_ARGS__))
#define CON_STR_28(element, ...) #element SEPERATOR MARCO_EXPAND(CON_STR_27(__VA_ARGS__))
#define CON_STR_29(element, ...) #element SEPERATOR MARCO_EXPAND(CON_STR_28(__VA_ARGS__))
#define CON_STR_30(element, ...) #element SEPERATOR MARCO_EXPAND(CON_STR_29(__VA_ARGS__))
#define CON_STR_31(element, ...) #element SEPERATOR MARCO_EXPAND(CON_STR_30(__VA_ARGS__))
#define CON_STR_32(element, ...) #element SEPERATOR MARCO_EXPAND(CON_STR_31(__VA_ARGS__))
#define CON_STR_33(element, ...) #element SEPERATOR MARCO_EXPAND(CON_STR_32(__VA_ARGS__))
#define CON_STR_34(element, ...) #element SEPERATOR MARCO_EXPAND(CON_STR_33(__VA_ARGS__))
#define CON_STR_35(element, ...) #element SEPERATOR MARCO_EXPAND(CON_STR_34(__VA_ARGS__))
#define CON_STR_36(element, ...) #element SEPERATOR MARCO_EXPAND(CON_STR_35(__VA_ARGS__))
#define CON_STR_37(element, ...) #element SEPERATOR MARCO_EXPAND(CON_STR_36(__VA_ARGS__))
#define CON_STR_38(element, ...) #element SEPERATOR MARCO_EXPAND(CON_STR_37(__VA_ARGS__))
#define CON_STR_39(element, ...) #element SEPERATOR MARCO_EXPAND(CON_STR_38(__VA_ARGS__))
#define CON_STR_40(element, ...) #element SEPERATOR MARCO_EXPAND(CON_STR_39(__VA_ARGS__))
#define CON_STR_41(element, ...) #element SEPERATOR MARCO_EXPAND(CON_STR_40(__VA_ARGS__))
#define CON_STR_42(element, ...) #element SEPERATOR MARCO_EXPAND(CON_STR_41(__VA_ARGS__))
#define CON_STR_43(element, ...) #element SEPERATOR MARCO_EXPAND(CON_STR_42(__VA_ARGS__))
#define CON_STR_44(element, ...) #element SEPERATOR MARCO_EXPAND(CON_STR_43(__VA_ARGS__))
#define CON_STR_45(element, ...) #element SEPERATOR MARCO_EXPAND(CON_STR_44(__VA_ARGS__))
#define CON_STR_46(element, ...) #element SEPERATOR MARCO_EXPAND(CON_STR_45(__VA_ARGS__))
#define CON_STR_47(element, ...) #element SEPERATOR MARCO_EXPAND(CON_STR_46(__VA_ARGS__))
#define CON_STR_48(element, ...) #element SEPERATOR MARCO_EXPAND(CON_STR_47(__VA_ARGS__))
#define CON_STR_49(element, ...) #element SEPERATOR MARCO_EXPAND(CON_STR_48(__VA_ARGS__))
#define CON_STR_50(element, ...) #element SEPERATOR MARCO_EXPAND(CON_STR_49(__VA_ARGS__))
#define CON_STR_51(element, ...) #element SEPERATOR MARCO_EXPAND(CON_STR_50(__VA_ARGS__))
#define CON_STR_52(element, ...) #element SEPERATOR MARCO_EXPAND(CON_STR_51(__VA_ARGS__))
#define CON_STR_53(element, ...) #element SEPERATOR MARCO_EXPAND(CON_STR_52(__VA_ARGS__))
#define CON_STR_54(element, ...) #element SEPERATOR MARCO_EXPAND(CON_STR_53(__VA_ARGS__))
#define CON_STR_55(element, ...) #element SEPERATOR MARCO_EXPAND(CON_STR_54(__VA_ARGS__))
#define CON_STR_56(element, ...) #element SEPERATOR MARCO_EXPAND(CON_STR_55(__VA_ARGS__))
#define CON_STR_57(element, ...) #element SEPERATOR MARCO_EXPAND(CON_STR_56(__VA_ARGS__))
#define CON_STR_58(element, ...) #element SEPERATOR MARCO_EXPAND(CON_STR_57(__VA_ARGS__))
#define CON_STR_59(element, ...) #element SEPERATOR MARCO_EXPAND(CON_STR_58(__VA_ARGS__))
#define CON_STR_60(element, ...) #element SEPERATOR MARCO_EXPAND(CON_STR_59(__VA_ARGS__))
#define CON_STR_61(element, ...) #element SEPERATOR MARCO_EXPAND(CON_STR_60(__VA_ARGS__))
#define CON_STR_62(element, ...) #element SEPERATOR MARCO_EXPAND(CON_STR_61(__VA_ARGS__))
#define CON_STR_63(element, ...) #element SEPERATOR MARCO_EXPAND(CON_STR_62(__VA_ARGS__))
#define CON_STR_64(element, ...) #element SEPERATOR MARCO_EXPAND(CON_STR_63(__VA_ARGS__))
#define CON_STR_65(element, ...) #element SEPERATOR MARCO_EXPAND(CON_STR_64(__VA_ARGS__))
#define CON_STR_66(element, ...) #element SEPERATOR MARCO_EXPAND(CON_STR_65(__VA_ARGS__))
#define CON_STR_67(element, ...) #element SEPERATOR MARCO_EXPAND(CON_STR_66(__VA_ARGS__))
#define CON_STR_68(element, ...) #element SEPERATOR MARCO_EXPAND(CON_STR_67(__VA_ARGS__))
#define CON_STR_69(element, ...) #element SEPERATOR MARCO_EXPAND(CON_STR_68(__VA_ARGS__))
#define CON_STR_70(element, ...) #element SEPERATOR MARCO_EXPAND(CON_STR_69(__VA_ARGS__))
#define CON_STR_71(element, ...) #element SEPERATOR MARCO_EXPAND(CON_STR_70(__VA_ARGS__))
#define CON_STR_72(element, ...) #element SEPERATOR MARCO_EXPAND(CON_STR_71(__VA_ARGS__))
#define CON_STR_73(element, ...) #element SEPERATOR MARCO_EXPAND(CON_STR_72(__VA_ARGS__))
#define CON_STR_74(element, ...) #element SEPERATOR MARCO_EXPAND(CON_STR_73(__VA_ARGS__))
#define CON_STR_75(element, ...) #element SEPERATOR MARCO_EXPAND(CON_STR_74(__VA_ARGS__))
#define CON_STR_76(element, ...) #element SEPERATOR MARCO_EXPAND(CON_STR_75(__VA_ARGS__))
#define CON_STR_77(element, ...) #element SEPERATOR MARCO_EXPAND(CON_STR_76(__VA_ARGS__))
#define CON_STR_78(element, ...) #element SEPERATOR MARCO_EXPAND(CON_STR_77(__VA_ARGS__))
#define CON_STR_79(element, ...) #element SEPERATOR MARCO_EXPAND(CON_STR_78(__VA_ARGS__))
#define CON_STR_80(element, ...) #element SEPERATOR MARCO_EXPAND(CON_STR_79(__VA_ARGS__))
#define CON_STR_81(element, ...) #element SEPERATOR MARCO_EXPAND(CON_STR_80(__VA_ARGS__))
#define CON_STR_82(element, ...) #element SEPERATOR MARCO_EXPAND(CON_STR_81(__VA_ARGS__))
#define CON_STR_83(element, ...) #element SEPERATOR MARCO_EXPAND(CON_STR_82(__VA_ARGS__))
#define CON_STR_84(element, ...) #element SEPERATOR MARCO_EXPAND(CON_STR_83(__VA_ARGS__))
#define CON_STR_85(element, ...) #element SEPERATOR MARCO_EXPAND(CON_STR_84(__VA_ARGS__))
#define CON_STR_86(element, ...) #element SEPERATOR MARCO_EXPAND(CON_STR_85(__VA_ARGS__))
#define CON_STR_87(element, ...) #element SEPERATOR MARCO_EXPAND(CON_STR_86(__VA_ARGS__))
#define CON_STR_88(element, ...) #element SEPERATOR MARCO_EXPAND(CON_STR_87(__VA_ARGS__))
#define CON_STR_89(element, ...) #element SEPERATOR MARCO_EXPAND(CON_STR_88(__VA_ARGS__))
#define CON_STR_90(element, ...) #element SEPERATOR MARCO_EXPAND(CON_STR_89(__VA_ARGS__))
#define CON_STR_91(element, ...) #element SEPERATOR MARCO_EXPAND(CON_STR_90(__VA_ARGS__))
#define CON_STR_92(element, ...) #element SEPERATOR MARCO_EXPAND(CON_STR_91(__VA_ARGS__))
#define CON_STR_93(element, ...) #element SEPERATOR MARCO_EXPAND(CON_STR_92(__VA_ARGS__))
#define CON_STR_94(element, ...) #element SEPERATOR MARCO_EXPAND(CON_STR_93(__VA_ARGS__))
#define CON_STR_95(element, ...) #element SEPERATOR MARCO_EXPAND(CON_STR_94(__VA_ARGS__))
#define CON_STR_96(element, ...) #element SEPERATOR MARCO_EXPAND(CON_STR_95(__VA_ARGS__))
#define CON_STR_97(element, ...) #element SEPERATOR MARCO_EXPAND(CON_STR_96(__VA_ARGS__))
#define CON_STR_98(element, ...) #element SEPERATOR MARCO_EXPAND(CON_STR_97(__VA_ARGS__))
#define CON_STR_99(element, ...) #element SEPERATOR MARCO_EXPAND(CON_STR_98(__VA_ARGS__))
#define CON_STR_100(element, ...) #element SEPERATOR MARCO_EXPAND(CON_STR_99(__VA_ARGS__))
#define CON_STR_101(element, ...) #element SEPERATOR MARCO_EXPAND(CON_STR_100(__VA_ARGS__))
#define CON_STR_102(element, ...) #element SEPERATOR MARCO_EXPAND(CON_STR_101(__VA_ARGS__))
#define CON_STR_103(element, ...) #element SEPERATOR MARCO_EXPAND(CON_STR_102(__VA_ARGS__))
#define CON_STR_104(element, ...) #element SEPERATOR MARCO_EXPAND(CON_STR_103(__VA_ARGS__))
#define CON_STR_105(element, ...) #element SEPERATOR MARCO_EXPAND(CON_STR_104(__VA_ARGS__))
#define CON_STR_106(element, ...) #element SEPERATOR MARCO_EXPAND(CON_STR_105(__VA_ARGS__))
#define CON_STR_107(element, ...) #element SEPERATOR MARCO_EXPAND(CON_STR_106(__VA_ARGS__))
#define CON_STR_108(element, ...) #element SEPERATOR MARCO_EXPAND(CON_STR_107(__VA_ARGS__))
#define CON_STR_109(element, ...) #element SEPERATOR MARCO_EXPAND(CON_STR_108(__VA_ARGS__))
#define CON_STR_110(element, ...) #element SEPERATOR MARCO_EXPAND(CON_STR_109(__VA_ARGS__))
#define CON_STR_111(element, ...) #element SEPERATOR MARCO_EXPAND(CON_STR_110(__VA_ARGS__))
#define CON_STR_112(element, ...) #element SEPERATOR MARCO_EXPAND(CON_STR_111(__VA_ARGS__))
#define CON_STR_113(element, ...) #element SEPERATOR MARCO_EXPAND(CON_STR_112(__VA_ARGS__))
#define CON_STR_114(element, ...) #element SEPERATOR MARCO_EXPAND(CON_STR_113(__VA_ARGS__))
#define CON_STR_115(element, ...) #element SEPERATOR MARCO_EXPAND(CON_STR_114(__VA_ARGS__))
#define CON_STR_116(element, ...) #element SEPERATOR MARCO_EXPAND(CON_STR_115(__VA_ARGS__))
#define CON_STR_117(element, ...) #element SEPERATOR MARCO_EXPAND(CON_STR_116(__VA_ARGS__))
#define CON_STR_118(element, ...) #element SEPERATOR MARCO_EXPAND(CON_STR_117(__VA_ARGS__))
#define CON_STR_119(element, ...) #element SEPERATOR MARCO_EXPAND(CON_STR_118(__VA_ARGS__))
#define CON_STR_120(element, ...) #element SEPERATOR MARCO_EXPAND(CON_STR_119(__VA_ARGS__))
#define RSEQ_N() \
		 119,118,117,116,115,114,113,112,111,110,\
		 109,108,107,106,105,104,103,102,101,100,\
		 99,98,97,96,95,94,93,92,91,90, \
		 89,88,87,86,85,84,83,82,81,80, \
		 79,78,77,76,75,74,73,72,71,70, \
		 69,68,67,66,65,64,63,62,61,60, \
         59,58,57,56,55,54,53,52,51,50, \
         49,48,47,46,45,44,43,42,41,40, \
         39,38,37,36,35,34,33,32,31,30, \
         29,28,27,26,25,24,23,22,21,20, \
         19,18,17,16,15,14,13,12,11,10, \
         9,8,7,6,5,4,3,2,1,0

#define ARG_N( \
         _1, _2, _3, _4, _5, _6, _7, _8, _9,_10, \
         _11,_12,_13,_14,_15,_16,_17,_18,_19,_20, \
         _21,_22,_23,_24,_25,_26,_27,_28,_29,_30, \
         _31,_32,_33,_34,_35,_36,_37,_38,_39,_40, \
         _41,_42,_43,_44,_45,_46,_47,_48,_49,_50, \
         _51,_52,_53,_54,_55,_56,_57,_58,_59,_60, \
		 _61,_62,_63,_64,_65,_66,_67,_68,_69,_70, \
		 _71,_72,_73,_74,_75,_76,_77,_78,_79,_80, \
		 _81,_82,_83,_84,_85,_86,_87,_88,_89,_90, \
		 _91,_92,_93,_94,_95,_96,_97,_98,_99,_100, \
		 _101,_102,_103,_104,_105,_106,_107,_108,_109,_110, \
         _111,_112,_113,_114,_115,_116,_117,_118,_119,N, ...) N

#define MARCO_EXPAND(...)                 __VA_ARGS__
#define APPLY_VARIADIC_MACRO(macro, ...)  MARCO_EXPAND(macro(__VA_ARGS__))

#define ADD_REFERENCE(t)        std::reference_wrapper<decltype(t)>(t)
#define ADD_REFERENCE_CONST(t)  std::reference_wrapper<std::add_const_t<decltype(t)>>(t)
#define OBJECT(t)          t//std::make_pair(#t, ADD_REFERENCE(t))
//#define OBJECT_CONST(t)    std::make_pair(#t, ADD_REFERENCE_CONST(t))
#define MAKE_NAMES(...) #__VA_ARGS__,

//#define MAKE_TUPLE_CONST(...)   auto tuple() const { return std::make_tuple(__VA_ARGS__); }

//note use MACRO_CONCAT like A##_##B direct may cause marco expand error
#define MACRO_CONCAT(A, B) MACRO_CONCAT1(A, B)
#define MACRO_CONCAT1(A, B) A##_##B

#define MAKE_ARG_LIST(N, op, arg, ...) \
        MACRO_CONCAT(MAKE_ARG_LIST, N)(op, arg, __VA_ARGS__)

#define GET_ARG_COUNT_INNER(...)    MARCO_EXPAND(ARG_N(__VA_ARGS__))
#define GET_ARG_COUNT(...)          GET_ARG_COUNT_INNER(__VA_ARGS__, RSEQ_N())

#define MAKE_STR_LIST(...) \
    MACRO_CONCAT(CON_STR, GET_ARG_COUNT(__VA_ARGS__))(__VA_ARGS__)

template<typename...> using void_t = void;

template<typename> struct Members {};

#define MAKE_META_DATA_IMPL(STRUCT_NAME, ...)\
template<>struct Members<STRUCT_NAME>{\
    constexpr decltype(auto) static apply(){\
        return std::make_tuple(__VA_ARGS__);\
    }\
    using type = void;\
    constexpr static const char *name = #STRUCT_NAME;\
    constexpr static const size_t value = GET_ARG_COUNT(__VA_ARGS__);\
    constexpr static const std::array<const char*, value>& arr = arr_##STRUCT_NAME;\
};

#define MAKE_META_DATA(STRUCT_NAME, N, ...) \
    constexpr std::array<const char*, N> arr_##STRUCT_NAME = {MARCO_EXPAND(MACRO_CONCAT(CON_STR, N)(__VA_ARGS__))};\
    MAKE_META_DATA_IMPL(STRUCT_NAME, MAKE_ARG_LIST(N, &STRUCT_NAME::OBJECT, __VA_ARGS__))

//MAKE_TUPLE_CONST(MAKE_ARG_LIST(N, OBJECT, __VA_ARGS__))

#define REFLECTION(STRUCT_NAME, ...) \
MAKE_META_DATA(STRUCT_NAME, GET_ARG_COUNT(__VA_ARGS__), __VA_ARGS__)

template <typename T, typename = void>
struct is_reflection : std::false_type
{
};

// this way of using SFINEA is type reference and cv qualifiers immuned
template <typename T>
struct is_reflection<T, void_t<
                typename Members<std::remove_const_t <std::remove_reference_t<T>>>::type
        >> : std::true_type
{
};

//template<size_t I, typename F, typename T>
//std::enable_if_t<is_reflection<T>::value> apply_value(F&& f, T&& t, bool is_last);

template<size_t I, typename F, typename T>
void apply_value(F&& f, T&& t, bool is_last)
{
    std::forward<F>(f)(std::forward<T>(t), I, is_last);
};

//template<size_t I, typename F, typename T>
//constexpr void for_each_impl(F&&, T&&, bool is_last, void_t<typename Members<std::remove_const_t <std::remove_reference_t<T>>>::type> *);

//template<size_t I, typename F, typename T>
//constexpr void for_each_impl(F&& f, T&& t, bool is_last, ...)
//{
//    std::forward<F>(f)(std::forward<T>(t), I, is_last);
//    //render_json_value<I>(std::forward<F>(f), std::forward<T>(t), is_last);
//}

template<size_t I, typename F, typename T>
constexpr void for_each_impl(F&& f, T&&t, bool is_last, void_t<typename Members<std::remove_const_t <std::remove_reference_t<T>>>::type> *)
{
    using M = Members<std::remove_const_t <std::remove_reference_t<T>>>;
    apply(std::forward<F>(f), std::forward<T>(t), M::apply(), std::make_index_sequence<M::value>{});
}

//-------------------------------------------------------------------------------------------------------------//
//-------------------------------------------------------------------------------------------------------------//
template<size_t I, typename F, typename F1, typename T>
std::enable_if_t<is_reflection<T>::value> apply_value(F&& f, F1&& f1, T&& t, bool is_last)
{
    std::forward<F1>(f1)(std::forward<T>(t), I, is_last);
}

template<size_t I, typename F, typename F1, typename T>
std::enable_if_t<!is_reflection<T>::value> apply_value(F&& f, F1&& f1, T&& t, bool is_last)
{
    std::forward<F>(f)(std::forward<T>(t), I, is_last);
}

//template<size_t I, typename F, typename F1, typename T>
//constexpr void for_each_impl(F&&, F1&&, T&&, bool is_last, void_t<typename Members<std::remove_const_t <std::remove_reference_t<T>>>::type> *);

template<size_t I, typename F, typename F1, typename T>
constexpr void for_each_impl(F&& f, F1&& f1, T&&t, bool is_last, void_t<typename Members<std::remove_const_t <std::remove_reference_t<T>>>::type> *)
{
    using M = Members<std::remove_const_t <std::remove_reference_t<T>>>;
    apply(std::forward<F>(f), std::forward<F1>(f1), std::forward<T>(t), M::apply(), std::make_index_sequence<M::value>{});
}

template<typename F, typename F1, typename T, typename... Rest,std::size_t I0, std::size_t... I>
constexpr void apply(F&& f, F1&& f1, T&&t, std::tuple<Rest...>&& tp, std::index_sequence<I0, I...>)
{
    apply_value<I0>(std::forward<F>(f), std::forward<F1>(f1), std::forward<T>(t).*(std::get<I0>(tp)), sizeof...(I)==0);
    apply(std::forward<F>(f), std::forward<F1>(f1), std::forward<T>(t), (std::tuple<Rest...>&&)tp, std::index_sequence<I...>{});
}

template<typename F, typename F1, typename T,typename... Rest>
constexpr void apply(F&& f, F1&&, T&& t, std::tuple<Rest...>&&, std::index_sequence<>)
{
}
//-------------------------------------------------------------------------------------------------------------//
//-------------------------------------------------------------------------------------------------------------//

template<typename F, typename T, typename... Rest,std::size_t I0, std::size_t... I>
constexpr void apply(F&& f, T&&t, std::tuple<Rest...>&& tp, std::index_sequence<I0, I...>)
{
    apply_value<I0>(std::forward<F>(f), std::forward<T>(t).*(std::get<I0>(tp)), sizeof...(I)==0);
    //for_each_impl<I0>(std::forward<F>(f), std::forward<T>(t).*(std::get<I0>(tp)), sizeof...(I)==0, (void *)nullptr);
    apply(std::forward<F>(f), std::forward<T>(t), (std::tuple<Rest...>&&)tp, std::index_sequence<I...>{});
}

template<typename F, typename T,typename... Rest>
constexpr void apply(F&& f, T&& t, std::tuple<Rest...>&&, std::index_sequence<>)
{
}

template<typename F, typename... Rest,std::size_t I0, std::size_t... I>
constexpr void apply_tuple(F&& f, std::tuple<Rest...>& tp, std::index_sequence<I0, I...>)
{
    apply_value<I0>(std::forward<F>(f), std::get<I0>(tp), sizeof...(I)==0);
    apply_tuple(std::forward<F>(f), tp, std::index_sequence<I...>{});
}

template<typename F, typename... Rest>
constexpr void apply_tuple(F&& f, std::tuple<Rest...>&, std::index_sequence<>)
{
}

template<size_t I, typename T>
constexpr decltype(auto) get(T&& t)
{
    using M = Members<std::remove_const_t <std::remove_reference_t<T>>>;
    return std::forward<T>(t).*(std::get<I>(M::apply()));
}

template<typename T, typename F>
constexpr std::enable_if_t<is_reflection<T>::value> for_each(T&& t, F&& f)
{
    for_each_impl<0>(std::forward<F>(f), std::forward<T>(t), false, (void *)nullptr);
}

template<typename T, typename F, typename F1>
constexpr std::enable_if_t<is_reflection<T>::value> for_each(T&& t, F&& f, F1&& f1)
{
    for_each_impl<0>(std::forward<F>(f), std::forward<F1>(f1), std::forward<T>(t), false, (void *)nullptr);
}

template<typename T, typename F>
constexpr std::enable_if_t<!is_reflection<T>::value> for_each(T&& tp, F&& f)
{
    apply_tuple(std::forward<F>(f), std::forward<T>(tp), std::make_index_sequence<std::tuple_size<std::remove_reference_t <T>>::value>{});
}

template<typename T, size_t  I>
constexpr const char* get_name()
{
    using M = Members<std::remove_const_t <std::remove_reference_t<T>>>;
    static_assert(I<M::value, "out of range");
    return M::arr[I];
}

template<typename T>
constexpr const char* get_name()
{
    using M = Members<std::remove_const_t <std::remove_reference_t<T>>>;
    return M::name;
}

template<typename T>
const char* get_name(size_t i)
{
    using M = Members<std::remove_const_t <std::remove_reference_t<T>>>;
    //if(i>=M::value)
    //    return "";

    return i >= M::value?"":M::arr[i];
}

template<typename T>
std::enable_if_t<is_reflection<T>::value, size_t> get_value()
{
    using M = Members<std::remove_const_t <std::remove_reference_t<T>>>;
    return M::value;
}

template<typename T>
std::enable_if_t<!is_reflection<T>::value, size_t> get_value()
{
	return 1;
}
#endif //UNTITLED3_REFLECTION_HPP
