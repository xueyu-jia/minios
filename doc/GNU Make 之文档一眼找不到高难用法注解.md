# 基础认知

一个完整的规则具有以下形式：

```
<targets>: <dependencies>
    <recipe>
```

targets 是该规则的目标，dependencies 是该规则的依赖项，其本身也是目标，recipe 是该规则的构建命令。

文件天然地被视为目标，当目标未被声明时，目标默认被视为同名的文件，反之目标始终指向 Makefile 内声明的目标。

recipe 通常用于指定目标是如何被构建的，但同样也可以用以表示该目标会执行的一系列动作，在这种情况下，该目标一般应当成为一个 .PHONY 伪目标。


# 规则重写

对于每个目标，recipe 至多被指定一次，多次指定视为重写，最后一次出现的 recipe 将成为该目标的构建命令。

对具体目标多次指定 recipe，最后一次出现的 recipe 将会被定义。

目标同时匹配模式和具体目标，总是定义为具体目标的 recipe。

目标同时匹配多个模式，第一个匹配的模式的 recipe 将会被定义。

# 依赖顺序

你可以多次通过仅指定 `<targets>: <dependencies>` 来为目标追加该依赖列表，这些依赖项默认会被追加到目标当前依赖列表的末尾。

recipe 的定义不会中断依赖列表，recipe 总是在全部依赖追加后才会被执行。

存在 recipe 定义的声明将会使得本次依赖项追加到当前依赖列表的最前面，无论该次 recipe 重写与否。

示例：

```plain
a: b c
a: f djw
a:
    @echo $^
a: ccc
.PHONY: a b c f ccc djw
```

```plain
a: b c
a: f djw
    @echo $^
a:
a: ccc
.PHONY: a b c f ccc djw
```

```plain
a: b c
a: f djw
    @echo $^
a:
a: ccc
    @echo $^
.PHONY: a b c f ccc djw
```

执行 `make a`，三次的输出分别是 `b c f djw ccc` `f djw b c ccc` `ccc f djw b c`。

# 依赖继承

多次声明的规则会将依赖项全部集合到一起，这意味着可以很轻易地将依赖声明和构建规则分离。

借助这一点，这看上去似乎会形成两种相关的主流用法。

其一，在 Makefile 的各个地方声明依赖，最终声明一个无依赖的目标用以定义 recipe，就像：

```plain
a:
    @echo $^

a: b c
a: main.c
a: ccc
a: d
```

这很显然适用于无所谓依赖顺序的情况，或者对于依赖的添加顺序非常清晰或者严格。

其二，在 Makefile 的各个地方声明依赖，最终声明一个带依赖的目标用于定义 recipe，就像：

```plain
a: main.c
    @echo $< $^

a: b c
a: main.c
a: ccc
a: d
```

这就更适用于依赖较为分散复杂，但是又只格外关心其中一部分依赖的情况。

需要注意的是，并非所有的依赖都会被继承。

能够被继承的依赖必须追加到一个具体的或可被立即求值的目标中，为模式目标追加的依赖并不会被继承到模式匹配到的目标中。

例如对于目标 dummy，存在模式 du%y，则 du%y 的依赖不会继承到 dummy 最终定义的 recipe 时的依赖列表中。

> 其实这非常好理解，毕竟具体目标 dummy 的 recipe 定义时，会直接忽略 du%y 模式，即使该模式匹配了。

反之，dummy 的依赖项将会继承到匹配的模式 du%y 中。

不妨令 dummy 的依赖项为 deps，du%y 的匹配项为 pat_deps，则会存在三种情形。

其一，dummy 的 recipe 被最终定义，则 dummy 的依赖项仅为 deps。

其二，du%y 的 recipe 被最终定义，则 dummy 的依赖项为 deps + pat_deps。

其三，recipe 均未定义，则 dummy 的最终依赖项为 deps。

前两种情形很好构造，而为了验证第三种情形，可以尝试：

```plain
kawaii: aa
%waii: bb
aa:
	@echo $@
bb:
	@echo $@
```

执行 `make kawaii`，得到结果 `aa`。

# 模式匹配的弊端

```plain
%.c.obj: %.c
    @gcc -c -o $@ $<
```

可以非常简单地写出以上规则来实现任意 .c 文件的编译，但也仅限于此了。

通配符 % 只作用于 targets 和 dependencies 之间的替换，你无法取得 % 的实际值，也无法通过 recipe 中能用的 $@ 来取得当前目标名。

相关的通配符在内建命令 patsubst 中存在，但是在 dependencies 中使用它，其参数内可能存在的 % 并不会指代该模式匹配的对应值，而是 patsubst 命令本身的另一个嵌套的通配符。

总之，我的意思是，目标的模式匹配是尽头之一，你没法通过它再做进一步处理了。

# 多目标规则

很显然，这一点在 [基础认知](#基础认知) 中已经暗示了，对于多个目标同时声明，你可以作展开理解。

例如：

```plain
a b c: dep1 dep2 dep3
```

可以视作：

```plain
a: dep1 dep2 dep3
b: dep1 dep2 dep3
c: dep1 dep2 dep3
```

看上去很奇怪没必要？可以先看看 GNU Make 官方文档里的例子：

```plain
%.tab.c %.tab.h: %.y
    bison -d $<
```

众所周知，bison 会直接生成两个目标，利用多目标规则的写法就可以一次搞定。

不过，这仅仅是常规用法之一，结合之前提到的不难发现这完全可以打造为一个更高级的模式匹配。

通过目标模式匹配得到的多个目标名匹配同样可以借助 wildcard 等其他方法得到，只不过这时候的思路就不是目标名到依赖的模式替换了，而是借助确定可知的目标列表作预处理。

举一个极端的例子，目标 t 需要依赖的目标是 `{ hash(1,t) hash(2,t) ... hash(n,t) }`，换作模式匹配这简直痴人说梦，而手动写上依赖又太过劳累。更何况，一个目标 t 尚好，可若是服从该规则的目标 t 是一组呢？

此时，利用 [依赖继承](#依赖继承) 和 [多目标规则](#多目标规则) 就可以很容易写出以下规则：

```plain
n = 64
hash = $(shell printf '%03d-%s' $(1) $(2) | md5sum | cut -c 1-32)

TARGETS := $(wildcard *.bin)

define declare-target-deps
$(1): $(foreach i,$(shell seq 1 $(n)),$(call hash,$(i),$(1)))
endef
$(foreach t,$(TARGETS),$(eval $(call declare-target-deps,$(t))))

$(TARGETS):
	@build-target -o $@ $^

all: $(TARGETS)
```

执行 `make all`，则可构建这一组拥有绝顶复杂且神金的依赖的 *.bin 目标。

> 此处为 target 添加的依赖为 64 个名字是 md5 的文件，由于咱其实并没有这些东东，所以你要跑的话可以在 define 里面把这一组依赖声明为伪目标。

# 所谓函数

方便起见，此处将内建命令和自定义方法统称为函数。

内建命令各有各的花哨，除了只跟字符处理相关的，剩余命令几乎无法用自定义方法复刻。

而对于自定义命令，其处理方式是非常粗暴的：参数先行展开，其次展开命令。

在这里面，重点是：展开。

像什么？没错，是宏。

说到底压根就没有什么函数的高级抽象，一切都近乎于是个宏。

只不过比起预处理的宏定义，GNU Make 的实现还是有稍许差别，详情见后。

在自定义命令时总是会用到 $(1) $(2) 这些代表对应位置参数的表示，而这些就是由内建命令 call 提供的。

那么其实就应该明了了——反正是调用方的 call 给的，并且函数也只不过是将其展开而已，那么为什么必须得用 define 语句块呢？

你会发现正常赋值也是一样的。

```plain
define foo
    $(info $(1))
endef
```

比起上面这坨看上去更正式的东西，`foo = $(info $(1))` 与其完全没有任何差别。

再更进一步，除开 $(1) $(2) 这些参数外，call 与直接变量引用并没有什么区别。

# 所谓展开

GNU Make 对换行和 TAB 极其敏感，敏感到了成为了其格式的一部分的程度。

任何命令调用必须是一行，任何 recipe 的每个步骤必须是一行。

你可以使用 backslash \ 换行，但这只不过是将下一行拼接到当前行。

你唯一可以名正言顺地换行的地方是 **展开到 eval 或非 recipe 处的变量替换和 call 调用** 的 define 语句块。

当然，更主要的是要牢记“展开”这个关键。

反正是展开，为什么非得定义一个名义为函数的东西呢？

要展开到 recipe，就可以定义 recipe 片段。

要展开到 eval，就可以定义 rules。

要展开到 call，就定义一个使用了参数的方法实现。

没什么不能做的。

# 局部变量

GNU Make 毕竟不是编程语言，没有那些杂七杂八的东西。

例如当你定义一个变量时，即便是通过 eval，他们也总是会暴露在全局。

一般而言这是没啥问题的，但是总会遇到令人困扰的情况，例如，你设计了一个通过 make 中定义的变量的值来像 CMake 的 configure_file 那样 config 文件。

这时候并不被期待的一些脏的变量就很容易会导致一些潜在的问题。

很显然，这需要局部变量。

但是又很显然，并没有这种东东。

但是，仔细想想除了直接定义和 eval 定义外，还存在一个特别的可以声明变量的内建命令：foreach！

$(foreach var,list,block) 将会令 var 按顺序取遍 list 中的 word 并展开 block。

假设这个 var 就取名叫 var，稍加试探就会发现，在 foreach 之后使用内建命令 origin 获取 var 的类型，会得到一个 undefined。

这不就是那不存在的局部变量吗？！

于是乎，给 foreach 稍稍打扮打扮就能构造出一个 scope 的用法，如：

```plain
# with_local_var <var-name> <default-value> <block>
with_local_var = $(foreach $(1),$(2),$(call $(3),$(1)))

name = vivokfc

$(info $(name))
func = $(info $(1) = $($(1)))
$(call with_local_var,name,foo,func)
$(info $(name))
```

可以得到预期结果：

```plain
vivokfc
name = foo
vivokfc
```

然而，需要注意的是这个“局部变量”其实只是个无敌削弱版的，只读且只能在实际嵌套层可见。

即使是在 foreach 内部，一旦你试图使用 eval 对其赋值，写入的也是全局的同名变量，这一赋值并不会对该 local var 有任何影响，它始终严格按照 foreach 给的 list 取值。

而所谓在实际嵌套层可见，其实这只是个反直觉但非常正常合理的现象，毕竟 foreach 处才是真正的局部作用域。

在 [所谓函数](#所谓函数) 中提到了，参数先展开，其次命令展开。在命令展开之前就尝试取了所谓局部变量的值，自然取到的不可能是展开之后才会从 foreach 的 list 取到的值。

这一现象导致的结果就是为了保证足够的灵活度，基本不可能对 foreach 进行封装使其表现得更像是一个专门定义局部变量的方法。

不过总之，能实现就已经很好了。

# 更常见的循环

根据 [所谓函数](#所谓函数) [所谓展开](#所谓展开) [局部变量](#局部变量) 三节其实可以感受到，GNU Make 中是不擅长甚至没法处理一个动态的控制流的。

ifeq 之流受到的限制非常之大，首先依赖非常敏感的换行，其次优先级并非足够高——如果你尝试在 define 中使用 ifeq 块，你很容易就会得到一个错误。

不过除了块语句，还有一个内建命令 if 可以使用。恰当地使用它也能达成不错的分支控制效果。

既然分支差不多了，就该探讨下循环的问题了。

局部变量只读意味着必须使用全局变量进行控制，但由于 foreach 会严格地遍历 list，于是中途 break 就是难以实现的操作。

但比起这些，令人更欢喜的可能还是这样一个循环语句块：

```plain
for (int i = 0; i < 10; ++i) {
    do_something(arr[i]);
}
```

循环无法控制，便要想办法构造一个期望的循环过程。

以上述为例，很容易想到利用 $(shell seq 0 9) 得到序列，然后使 foreach 取遍。

> 其实这是很常用的元编程手段。

另一个实现常用循环必须要注意到但实则很容易忽视且会因觉得过于正常而无所谓的点是：foreach 是顺序取遍展开的。

按顺序展开嘛，很正常，这有什么了不起的？

这可太了不起了！

你说巧不巧，eval 也是顺序执行的！于是便完全可以令各元素并不相关的 foreach 成为更容易相关的 for！

比如，利用 foreach 来求个 fibonacci 数列第 11 项：

```plain
define _fib_shift
    $(eval $(2) = $(word 1,$(1)))
    $(eval $(3) = $(word 2,$(1)))
endef
define fib
	$(eval prev := 1)
    $(eval result := 1)
    $(if $(shell [ $(1) -gt 2 ] && echo 1), \
		$(foreach x,$(shell seq 3 $(1)),    \
			$(call _fib_shift,$(result) $(shell echo $$[ $(prev) + $(result) ]),prev,result)),)
endef

$(call fib,11)
$(info $(result))
```

非常的“循环”不是吗？
