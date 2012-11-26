#include "Packet.h"

class Event
{
	public:
		enum Type
		{
			None=0,
			Message
		};
	private:
		Type m_Type;
		dtglib::C_Packet m_Packet;
	public:
		Event() : m_Type(None), next(NULL) {}
		Event(Type t) : m_Type(t), next(NULL) {}
		~Event()
		{
			Event* n=next;
			while(n)
			{
				Event* nn=n->next;
				delete n;
				n=nn;
			}
		}
		Event* next;
		void SetType(const Type& t) { m_Type=t; }
		Type Type() const { return m_Type; }
		
		template<class T> Event& operator<<(const T& rhs)
		{
			m_Packet << rhs;
			return *this;
		}
		const wchar_t* Data() const { return (const wchar_t*)m_Packet.M_RawData(); }
};
