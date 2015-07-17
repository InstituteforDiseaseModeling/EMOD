/**********************************************

License: BSD
Project Webpage: http://cajun-jsonapi.sourceforge.net/
Author: Terry Caton, tcaton(a)hotmail.com

TODO: additional documentation. 

***********************************************/

namespace json
{


// just a declaration, specializations below
template <typename ElementType>
class ElementCastImp;

// reference to non-const
template <typename ElementTypeT>
class ElementCastImp<ElementTypeT&>
{
public:
   typedef ElementTypeT& ReturnType;

   static ReturnType cast_i(Element& element) {
      if (element.Type() != ElementTypeT::Type_i())
         throw Exception("Bad json_cast");
      return static_cast<ReturnType>(element);
   }
};

template <typename ElementTypeQualifiedT>
ElementTypeQualifiedT json_cast(Element& element) {
   return ElementCastImp<ElementTypeQualifiedT>::cast_i(element);   
}


// reference to const
template <typename ElementTypeT>
class ElementCastImp<const ElementTypeT&>
{
public:
   typedef const ElementTypeT& ReturnType;

   static ReturnType cast_i(const Element& element) {
      if (element.Type() != ElementTypeT::Type_i())
         throw Exception("Bad json_cast");
      return static_cast<ReturnType>(element);
   }
};

template <typename ElementTypeQualifiedT>
ElementTypeQualifiedT json_cast(const Element& element) {
   return ElementCastImp<ElementTypeQualifiedT>::cast_i(element);   
}


// pointer to non-const
template <typename ElementTypeT>
class ElementCastImp<ElementTypeT*>
{
public:
   static ElementTypeT* cast_i(Element* pElement) {
      return (pElement->Type() == ElementTypeT::Type_i() ? static_cast<ElementTypeT*>(pElement) :
                                                       0);
   }
};

template <typename ElementTypeQualifiedT>
ElementTypeQualifiedT json_cast(Element* pElement) {
   return ElementCastImp<ElementTypeQualifiedT>::cast_i(pElement);   
}


// pointer to const
template <typename ElementTypeT>
class ElementCastImp<const ElementTypeT*>
{
public:
   static ElementTypeT* cast_i(const Element* pElement) {
      return (pElement->Type() == ElementTypeT::Type_i() ? static_cast<const ElementTypeT*>(pElement) :
                                                       0);
   }
};

template <typename ElementTypeQualifiedT>
ElementTypeQualifiedT json_cast(const Element* pElement) {
   return ElementCastImp<ElementTypeQualifiedT>::cast_i(pElement);   
}


} // End namespace