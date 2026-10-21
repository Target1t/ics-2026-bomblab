# BombLab

## Phase 1

`p1_compute` 将 64 位的 `challenge` 拆成三部分：

```c
uint32_t low  = (uint32_t)challenge;
uint32_t mid  = (uint32_t)(challenge >> 16);
uint32_t high = (uint32_t)(challenge >> 32);
```

低 32 位直接截取得到 `low`；先将 64 位整数右移 16 位或 32 位，再截取低 32 位，分别得到 `mid` 和 `high`。注意 `low` 与 `mid` 的位段有重叠，不能把它们理解为三个互不重叠的字段。

根据 `p1_step0`、`p1_step1`、`p1_step2` 的运算关系，依次计算：

```c
uint32_t a = (5 * low) ^ (high + 0xC1EF);
uint32_t b = 3 * (high - mid) + 0xF950;
uint32_t c = (b + 8 * low) ^ 0x9D6E;
uint32_t answer = c ^ a;
```

## Phase 2

主函数要求输入六个无符号整数。第一个值来自 challenge 低 32 位右移 24 位后的结果：

```c
values[0] = ((uint32_t)challenge >> 24) ^ 12;
```

循环中：

```asm
mov -0x40(%rbp,%rdx,4),%esi
lea (%rax,%rsi,2),%eax
```

第一条读取地址 `%rbp - 0x40 + %rdx * 4` 处的 4 字节整数。这里 `%rdx` 用作数组下标，乘数 4 对应一个 `uint32_t` 的大小。读取后 `%esi` 保存该数组元素

第二条 `lea` 计算 `%rax + 2 * %rsi`，将结果低 32 位写入 `%eax`。

```c
for (uint32_t i = 1; i < 6; i++) {
    uint32_t shift = 20 + 5 * (i - 1);
    uint32_t t = (uint32_t)((challenge >> shift) & 31);
    values[i] = 2 * values[i - 1] + (t ^ i);
}
```

## Phase 3

主函数中：

```asm
mov %r12d,%edi
and $0x7,%edi
cmp %edi,-0x18(%rbp)
jne 402275
```

函数入口处 `%rsi` 接收第二个参数 `challenge`，随后 `mov %rsi,%r12` 保存它。`%r12d` 是其低 32 位，`and $7,%edi` 保留最低三位。`-0x18(%rbp)` 是第一个输入整数的位置；不相等则跳到失败路径。因此第一个答案是 `challenge & 7`。

```asm
mov %r12,%rax
mov %r12,%rsi
shr $0x3,%rax
shr $0x20,%rsi
xor %eax,%esi
call p3_dispatch
```

两次 `mov` 分别复制 challenge。右移后，`xor %eax,%esi` 只对低 32 位进行异或，因此传给 `p3_dispatch` 的第二个参数是：

```c
uint32_t y = (uint32_t)(challenge >> 3)
           ^ (uint32_t)(challenge >> 32);
```

`p3_dispatch` 中的间接跳转为：

```asm
mov %edi,%eax
mov %esi,%edi
cmp $0x7,%eax
ja 402245
notrack jmp *0x4057e0(,%rax,8)
```

此时 `%eax` 保存选择分支的下标。最后一条指令从 `0x4057e0 + 8 * x` 读取跳转地址。

| x | 实际执行的函数 |
|---|---|
| 0 | `p3_case_1` |
| 1 | `p3_case_4` |
| 2 | `p3_case_3` |
| 3 | `p3_case_0` |
| 4 | `p3_case_2` |
| 5 | `p3_case_5` |
| 6 | `p3_case_7` |
| 7 | `p3_case_6` |

```c
switch (x) {
    case 0: answer = (y ^ 0x4E9D) + (y >> 2); break;
    case 1: answer = 5 * y + 0xEE44; break;
    case 2: answer = (4 * y) ^ 0xDECA; break;
    case 3: answer = 5 * y + 0x6E37; break;
    case 4: answer = 3 * y + 0x34C3; break;
    case 5: answer = (y ^ 0xDB36) + (y >> 2); break;
    case 6: answer = (4 * y) ^ 0xFD06; break;
    case 7: answer = 5 * y + 0x4F001; break;
}
```

## Phase 4

调用 `p4_walk` 之前，主函数依次设置四个参数：

```asm
mov %r12,%rcx
mov %r12,%rax
mov %r12d,%edi
xor %esi,%esi
shr $0x20,%rax
shr $0x7,%rcx
and $0x7f,%edi
mov $0x7f,%edx
xor %eax,%ecx
call p4_walk
```

按照前四个整数参数分别通过 `%edi`、`%esi`、`%edx`、`%ecx` 传递的约定，得到：

```c
uint32_t target = (uint32_t)challenge & 127;
uint32_t extra  = (uint32_t)(challenge >> 7)
                ^ (uint32_t)(challenge >> 32);
p4_walk(target, 0, 127, extra);
```

`p4_walk` 计算：

```asm
mov %edi,%eax
mov %edx,%edi
sub %esi,%edi
shr $1,%edi
add %esi,%edi
cmp %eax,%edi
je 402330
```

刚进入函数时，`%edi` 为 target，`%esi` 为 low，`%edx` 为 high。先将 target 保存到 `%eax`，再把 `%edi` 改为 `high - low`，右移一位并加上 low，因此 `%edi` 最终为：

```c
uint32_t mid = low + ((high - low) >> 1);
```

与 `%eax` 中保存的 target 相等时，直接执行：

```asm
lea 0x27(%rdi),%eax
xor %ecx,%eax
ret
```

这时 `%edi = mid`，`%ecx = extra`，所以返回 `(mid + 0x27) ^ extra`。

小于中点时进入左分支：

```asm
lea -0x1(%rdi),%edx
mov %eax,%edi
call p4_walk
lea (%rax,%rax,4),%eax
xor %ebx,%eax
xor $0x12,%eax
ret
```

```c
uint32_t mid = low + ((high - low) >> 1);
if (target == mid)
    return (mid + 0x27) ^ extra;
if (target < mid)
    return (5 * p4_walk(target, low, mid - 1, extra))
           ^ (mid + extra) ^ 0x12;
return (5 * p4_walk(target, mid + 1, high, extra))
       ^ (mid + extra) ^ 0x14;
```

## Phase 5

`p5_traverse` 首先通过 challenge 的低三位选择一个节点：

```asm
mov %rdi,%rax
and $0x7,%eax
mov 0x405b20(,%rax,8),%rdx
```

第一个参数 `%rdi` 是 challenge。`and $7,%eax` 得到 0～7 的索引，再从 `0x405b20 + 8 * index` 取出 8 字节指针，所以 `%rdx` 保存的是节点地址。

节点访问：

```asm
mov 0x4(%rdx),%esi
add (%rdx),%eax
mov 0x8(%rdx),%rdx
```

`0x4(%rdx)` 读出节点起始地址后第 4 字节处的 32 位整数，`(%rdx)` 读出起始位置的 32 位整数，`0x8(%rdx)` 读出 64 位下一节点地址。据此可按下面的布局理解节点：

```c
struct Node {
    uint32_t value;
    uint32_t tag;
    struct Node *next;
};
```

初始值计算中：

```asm
shr $0x6,%rax
xor %edi,%eax
xor $0x8939,%eax
lea (%rax,%rax,2),%eax
add (%rdx),%eax
xor %esi,%eax
```

指令先将 challenge 右移 6 位的低 32 位与 challenge 高 32 位异或，再异或 `0x8939`。

```c
uint32_t seed = (uint32_t)(challenge >> 6)
              ^ (uint32_t)(challenge >> 32)
              ^ 0x8939;
uint32_t result = (3 * seed + node->value) ^ node->tag;
```

循环中的：

```asm
mov 0x8(%rdx),%rdx
lea (%rax,%rax,2),%eax
mov 0x4(%rdx),%esi
add (%rdx),%eax
xor %esi,%eax
mov %esi,(%r8)
```

对应沿着 next 指针进入下一节点，再更新 `result = (3 * result + value) ^ tag`。最后一条把 tag 写到 `%r8` 指向的内存。

读取 `0x405b20` 处的指针表及各节点内容，顺序为：

```text
405c10 -> 405b90 -> 405b70 -> 405bb0 -> 405b80 -> 405bc0
       -> 405c00 -> 405bf0 -> 405be0 -> 405b60 -> 405ba0 -> 405bd0
```

## Phase 6

`p6_dispatch` 在进入时通过 `%rdi` 接收 challenge：

```asm
mov %rdi,%rbx
shr $0x6,%rdi
mov %rbx,%rax
shr $0x20,%rax
xor %eax,%edi
```

第一条保存原值；后面的移位和异或使第一个函数调用参数 `%edi` 等于：

```c
uint32_t x = (uint32_t)(challenge >> 6)
           ^ (uint32_t)(challenge >> 32);
```

接下来：

```asm
mov %rbx,%rax
and $0x38,%ebx
and $0x7,%eax
call *0x405dc0(,%rax,8)
```

`%eax = challenge & 7`，故第一次从 `0x405dc0 + 8 * index1` 取出函数地址。`%ebx = challenge & 0x38`，保存了第二次调用要使用的字节偏移，对应 `(challenge >> 3) & 7`。

用 `objdump -s` 读取指针表，再对照各函数的入口地址，得出索引对应关系：

| 索引 | 函数 |
|---|---|
| 0、1 | `p6_op_1` |
| 2 | `p6_op_5` |
| 3、7 | `p6_op_3` |
| 4 | `p6_op_2` |
| 5 | `p6_op_4` |
| 6 | `p6_op_0` |

例如 `p6_op_1` ：

```asm
lea 0x0(,%rdi,8),%edx
shl $0x5,%eax
sub %edi,%edx
add $0x432f,%eax
xor %edx,%eax
```

`lea` 得到 `8*x`，减去 `%edi` 中的 x 后得到 `7*x`；`shl $5` 得到 `32*x`，加常数再异或，因此计算 `(32*x + 0x432F) ^ (7*x)`。


## Honor

`secret_gate` 首先检查字符串长度：

```asm
call strlen
sub $0x24,%rax
cmp $0x7f,%rax
jbe 402710
```

`strlen` 返回长度到 `%rax`，减去十进制 36，再进行无符号小于等于 127 的比较，因此输入长度要在 36～163 字节之间。

`secret_copy` 中：

```asm
movzbl (%rsi,%rax,1),%edx
mov %dl,(%rdi,%rax,1)
add $0x1,%rax
test %dl,%dl
jne 4026a0
```

它从 `%rsi + index` 读取一个字节，再写到 `%rdi + index`，直到复制完结尾的 `\0`，因此输入字符串被原样复制。

`secret_gate` 之后执行：

```asm
cmpl $0x4e45504f,-0xb0(%rbp)
```

复制目标的起始地址是 `%rbp - 0xd0`；被检查的位置是 `%rbp - 0xb0`，两者相差 32 字节。整数 `0x4E45504F` 在内存中的字节为 `4F 50 45 4E`，即 `OPEN`。因此输入的第 32～35 字节是 `OPEN`。

```asm
lea -0xac(%rbp),%rdi
call secret_phase
```

这里传入的地址比复制目标起点偏移 36 字节，故 `secret_phase` 只处理 `OPEN` 后面的字符串。它调用 `valid_u32_fields` 并将数量设置为 3，说明后面还需要三个无符号整数。

第一个整数的比较：

```asm
mov %r13d,%ecx
and $0x3,%ecx
cmp %ecx,-0x2c(%rbp)
```

`%r13` 保存传入的 context。与 3 按位与后只保留最低两位，得到 `a = (uint32_t)context & 3`。

`secret_mix` 中：

```asm
mov %edi,%eax
rol $0x7,%eax
add $0x6d2b,%eax
xor $0x2ad8,%eax
ret
```

`rol $7` 是 32 位循环左移 7 位，等价于 `(x << 7) | (x >> 25)`。其后加 `0x6D2B` 再异或 `0x2AD8`，得到 mixed。

结合 `secret_phase` 的数据处理，设：

```c
uint32_t challenge_fold = (uint32_t)(challenge >> 32)
                        ^ (uint32_t)challenge;
uint32_t context_fold = (uint32_t)(context >> 32)
                      ^ (uint32_t)context;
uint32_t x = challenge_fold ^ context_fold;
uint32_t mixed = (((x << 7) | (x >> 25)) + 0x6D2B) ^ 0x2AD8;
uint32_t a = (uint32_t)context & 3;
uint32_t b = mixed ^ ((a * 0x6D2B) ^ (uint32_t)(context >> 16));
uint32_t c = b ^ (context_fold + a + 0x2AD8
                + (challenge_fold >> 7));
```


## 测试

完成代码后执行 `make test`，结果为：

```text
Public tests: 60/60
Secret public tests: 10/10
```

实验分析过程中使用 ChatGPT 5.6-Sol 辅助汇编阅读理解及反向函数结构探讨和MD文件组织整理。
