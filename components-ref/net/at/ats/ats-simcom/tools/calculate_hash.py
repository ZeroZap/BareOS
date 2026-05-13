import sys


def calculate_hash(at_name):
    value1 = 0
    value2 = 0

    AT_HASH_TABLE_ROW = (37)
    AT_HASH_TABLE_SPAN = (5)
    AT_MAX_CMD_NAME_LEN = (2 * AT_HASH_TABLE_SPAN)
    AT_MAX_CMD_HEAD_LEN = (AT_MAX_CMD_NAME_LEN + 3)

    ascii_char = 0
    if at_name[0] == 'a' or at_name[0] == 'A':
        if at_name[1] == 't' or at_name[1] == 'T':
            # caculate hash value after ("AT+" or "AT#") until entering ('=' or '?' or CR/LF/NULL)
            i = 3
            while ((at_name[i] != '=') and (at_name[i] != '?')
                    and (at_name[i] != 13) and (at_name[i] != 10)
                    and (at_name[i] != '\0')):
                if at_name[i].isupper():
                    ascii_char = ord(at_name[i]) - 65
                elif at_name[i].islower():
                    ascii_char = ord(at_name[i]) - 97
                elif at_name[i].isdigit():
                    ascii_char = ord(at_name[i]) - 30

                if i < (AT_HASH_TABLE_SPAN + 3):
                    # /* 0 ~ 4*/
                    value1 = value1*(AT_HASH_TABLE_ROW+1)+(ascii_char+1)
                elif i < (AT_MAX_CMD_NAME_LEN + 3):
                    # /* 5 ~ 9*/
                    value2 = value2*(AT_HASH_TABLE_ROW+1)+(ascii_char+1)

                i += 1

                if (i == len(at_name)):
                    break
    return value1, value2


# print(calculate_hash("AT+EGPSC"))

if __name__ == "__main__":

    print("(hash_value1, hash_value2) = ", (calculate_hash(sys.argv[1])))
