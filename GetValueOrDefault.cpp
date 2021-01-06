template <typename PointedClassMemberType, typename PointedClass>
PointedClassMemberType GetValueOrDefault(const PointedClass* ptr, const PointedClassMemberType PointedClass::* ClassMemberName)
{
	return GetValueOrDefault(ptr, ClassMemberName, PointedClassMemberType{});
}

template <typename PointedClassMemberType, typename PointedClass>
PointedClassMemberType GetValueOrDefault(const PointedClass* ptr, const PointedClassMemberType PointedClass::* ClassMemberName, PointedClassMemberType&& DefaultValue)
{
	return ptr == nullptr ?
		DefaultValue : ptr->*ClassMemberName;
}

// demonstration

#include <memory>
#include <iostream>
#include <string>

using namespace std;

class A
{
public:
	int dat{ 50 };
	unsigned int mt{ 900 };
	wstring words{ L"Words" };
};

class B
{
public:
	unique_ptr<A> aptr{ nullptr };
};

int main()
{
	A ConstructedA{};
	B BWithConstructedA{};
	BWithConstructedA.aptr = make_unique<A>(ConstructedA);
	B BWithoutConstructedA{};
	wcout << L"A.dat from B with a valid A pointer: " << BWithConstructedA.aptr->dat << endl;
	wcout << L"A.mt from B with a valid A pointer: " << BWithConstructedA.aptr->mt << endl;
	wcout << L"A.words from B with a valid A pointer: " << BWithConstructedA.aptr->words << endl << endl;
	wcout << L"Default-constructed A.dat from B with an A null pointer: " <<
		GetValueOrDefault(BWithoutConstructedA.aptr.get(), &A::dat) << endl;
	wcout << L"Overriden default A.mt from B with an A null pointer: " <<
		GetValueOrDefault(BWithoutConstructedA.aptr.get(), &A::mt, 42u) << endl;
	wcout << L"Overriden default A.words from B with an A null pointer: " <<
		GetValueOrDefault(BWithoutConstructedA.aptr.get(), &A::words, wstring{ L"forty-two" }) << endl;
}
