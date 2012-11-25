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
};
