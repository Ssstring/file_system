# 类型检测
# 输入类型有：整数和布尔型
# 输入: 一个表达式 以及 其返回值
# 任务: 根据输入的表达式，及返回值判断类型是否正确
# 1. 判断表达式内部是否正确，对于不同的函数，其要求也有不同，需要用一个dict来存储函数及其表达式
# 2. 判断返回值是否正确
# 3. 可能有多个表达式嵌套的可能
# 思路一: 修改lisp_to_c的代码，将一个个函数分割开来，然后再进行判断
# 思路二: 使用语法分析去实现


oper = {
    'add': {
        'params': ['int', 'int'],
        'ret': 'int'
    },
    'sub': {
        'params': ['int', 'int'],
        'ret': 'int'
    },
    'gt': {
        'params': ['int', 'int'],
        'ret': 'bool'
    },
    'lt': {
        'params': ['int', 'int'],
        'ret': 'bool'
    },
    'equ': {
        'params': ['int', 'int'],
        'ret': 'bool'
    },
    'and': {
        'params': ['bool', 'bool'],
        'ret': 'bool'
    },
    'or': {
        'params': ['bool', 'bool'],
        'ret': 'bool'
    },
    'not': {
        'params': ['bool'],
        'ret': 'bool'
    }
}

class parentheses:
    def __init__(self, l, r, d):
        self.l = l
        self.r = r
        self.d = d
    def __str__(self):
        return 'Parentheses: [l=%d, r=%d, d=%d]'% (self.l, self.r, self.d)

# 采用思路一: 可以使用括号匹配找出一个个括号对的位置，记录括号对深度，从而分开各个括号
def dispose_lisp(lisp: str):
    depth = 0
    t = []
    l = []
    d = []
    for index in range(len(lisp)):
        # print(index)
        if lisp[index] == '(':
          l.append(index)
          d.append(depth)
          depth+=1
        elif lisp[index] == ')':
            t.append(parentheses(l.pop(), index, d.pop()))
            depth-=1
    return t
# 获取每一个的返回值，传入一个字符串，注意传入的字符串中去除其本身的括号，递归调用
def check(para: str, l, r):
    t = oper[para]
    if l == t['params'][0] and r == t['params'][1]:
        return t['ret']
    return 'error'

def get_all_return(lisp: str):
    t = dispose_lisp(lisp)
    if len(t) > 0:
        para = lisp.split()[0]
        ans = []
        for i in t:
            if i.d == 0:
                ans.append(i)
        # 根据lisp前面的str判断参数数量
        if para == 'not':
            ret = get_all_return(lisp[ans[0].l+1, ans[0].r])
            if ret == 'bool':
                return 'bool'
            else:
                return 'error'
        else:
            left = get_all_return(lisp[ans[0].l+1, ans[0].r])
            right = get_all_return(lisp[ans[1].l+1, ans[1].r])
            return check(para, left, right)
    else:
        paras = lisp.split()
        if paras[0] == 'not':
            if paras[1] == 'T' or paras[1] == 'F':
                return 'bool'
            else:
                return 'error'
        else:
            left, right = '', ''
            if paras[1] == 'T' or paras[1] == 'F':
                left = 'bool'
            else:
                left = 'int'
            if paras[2] == 'T' or paras[2] == 'F':
                right = 'bool'
            else:
                right = 'int'
            return check(para, left, right)

            

def lisp_to_c(lisp:str):
    lisp = lisp.split('(')
    lisp = "".join(lisp)
    lisp = lisp.split()
    ans = ""
    flag = 0
    for i in lisp:
        ans+=i
        if i in oper.keys():
            ans+="("
            flag+=1
        elif flag > 0:
            flag -= 1
            ans += ", "
    print(ans)
    return ans    

s = '(gt (add 5 3) (sub 5 3))'
lisp_to_c(s)


print(get_all_return(s[1:-1]))