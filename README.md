## CppIdioms

### Pimpl

Pimpl(**P**ointer to **impl**ementation) 是一种减少代码依赖和编译时间的C++编程技巧，其基本思想是将一个外部可见类(visible class)的实现细节（一般是所有私有的非虚成员）放在一个单独的实现类(implementation class)中，而在可见类中通过一个私有指针来间接访问该实现类。

别名：编译防火墙

相关术语：handle/body, 桥接模式
