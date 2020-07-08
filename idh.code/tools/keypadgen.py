import argparse
import csv

DESCRIPTION = '''
Generate keypad map source file from csv
'''

def main():
    parser = argparse.ArgumentParser(description=DESCRIPTION)
    parser.add_argument("csv",help="csv file for keypad map")
    parser.add_argument("keypaddef",nargs="?",default='hal_keypad_def.h')
    args = parser.parse_args()
    reg = 0
    pins=[]
    cfg = {}
    with open(args.csv,"r") as fh:
         pinmap = csv.reader(fh)
         for col in pinmap:
            if not col[0]:
                continue
            n = -1
            for pin in col[1:]:
                n += 1
                if not pin:
                    continue
                pins.append(pin)
                cfg[pin] = (reg,n)
            reg +=1
    #print(pins)
    #print(cfg)

    with open(args.keypaddef,"w") as fh:
        fh.write("#ifndef _HAL_KEYPAD_DEF_\n")
        fh.write("#define _HAL_KEYPAD_DEF_\n\n\n")
        fh.write('//  Auto generated. Don\'t edit it manually!\n\n')
        fh.write('typedef enum \n{\n')
        fh.write('    KEY_MAP_POWER=0,\n')
        for pin in pins:
           number = cfg[pin]
           fh.write('    {}={},\n'.format(pin.upper(),number[0]*8+number[1]+1))
        fh.write('    KEY_MAP_MAX_COUNT,\n')
        fh.write('}keyMap_t;\n')
        fh.write("\n#endif\n\n")
if __name__ == "__main__":
    main()