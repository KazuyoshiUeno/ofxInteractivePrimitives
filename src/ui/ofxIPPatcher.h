#pragma once

#include "ofMain.h"

#include "ofxInteractivePrimitives.h"
#include "ofxIPStringBox.h"

#include <set>


namespace ofxInteractivePrimitives
{
	struct PortIdentifer;
	
	class Port;
	class PatchCord;
	
	class BaseMessage;
	
	template <typename T>
	class Message;
	
	typedef ofPtr<BaseMessage> MessageRef;
	
	class BasePatcher;
	
	struct NullParam {};
	
	template <typename T, typename P, typename V>
	class Patcher;
	
	template <typename T, typename ContextType, typename ParamType, typename InteractivePrimitiveType>
	struct AbstructWrapper;

	struct DelayedDeletable;
	
	typedef unsigned long TypeID;
	
	// TODO: more better RTTI method
	template <typename T>
	TypeID Type2Int()
	{
		const static unsigned int s = 0;
		return (TypeID) & s;
	};
	
	inline bool in_range(const int& a, const int& b, const int& c)
	{
		return a >= b && a < c;
	}
	
	static Port *patching_port;
	
	struct BaseWrapper;
}

#pragma mark - DelayedDeletable

struct ofxInteractivePrimitives::DelayedDeletable
{
public:
	
	DelayedDeletable() : will_delete(false) {}
	virtual ~DelayedDeletable() {}
	
	void delayedDelete()
	{
		if (will_delete) return;
		will_delete = true;
		
		addToDelayedDeleteQueue(this);
	}
	
	static void deleteQueue()
	{
		Queue &queue = getQueue();
		Queue::iterator it = queue.begin();
		
		while (it != queue.end())
		{
			DelayedDeletable *o = *it;
			delete o;
			it++;
		}
		
		queue.clear();
	}
	
protected:
	
	static void addToDelayedDeleteQueue(DelayedDeletable *o) { getQueue().insert(o); }
	
protected:
	
	bool getWillDelete() { return will_delete; }
	
private:
	
	typedef std::set<DelayedDeletable*> Queue;
	static Queue& getQueue() { static Queue queue; return queue; }
	
	bool will_delete;
	
};

#pragma mark - BaseMessage

class ofxInteractivePrimitives::BaseMessage : public DelayedDeletable
{
public:
	
	virtual ~BaseMessage() {}
	
	virtual bool isTypeOf() const { return false; }
	
	// TODO: add type checking
	template <typename T>
	Message<T>* cast() { return (Message<T>*) this; }
	
	void execute() {}
};

#pragma mark - Message

template <typename T>
class ofxInteractivePrimitives::Message : public BaseMessage
{
public:
	
	Message() : type(Type2Int<T>()), value(T()) {}
	Message(const T& value) : type(Type2Int<T>()), value(value) {}
	
	bool isTypeOf() const { return type == Type2Int<T>(); }
	const T& get() { return value; }
	bool set(const T& v) { value = v; }
	
	static MessageRef create(const T& v)
	{
		Message<T> *ptr = new Message<T>(v);
		return MessageRef(ptr);
	}
	
	static MessageRef create()
	{
		Message<T> *ptr = new Message<T>(T());
		return MessageRef(ptr);
	}
	
private:
	
	T value;
	TypeID type;
};

#pragma mark - PortIdentifer

struct ofxInteractivePrimitives::PortIdentifer
{
	enum Direction
	{
		INPUT,
		OUTPUT
	};
};

#pragma mark - PatchCord

class ofxInteractivePrimitives::PatchCord : public Node, public DelayedDeletable
{
	friend class Port;
	
public:
	
	PatchCord(Port *upstream_port, Port *downstream_port);
	~PatchCord() {}

	void disconnect();
	
	bool isValid() const { return upstream && downstream; }
	
	Port* getUpstream() const { return upstream; }
	Port* getDownstream() const { return downstream; }
	
	void draw();
	void hittest();
	
	void keyPressed(int key);
	
protected:
	
	Port* upstream;
	Port* downstream;
};

#pragma mark - Port

class ofxInteractivePrimitives::Port
{
	template <typename T, typename P, typename V>
	friend class Patcher;
	
public:
	
	Port(BasePatcher *patcher, int index, PortIdentifer::Direction direction);
	~Port()
	{
		cords.clear();
	}
	
	void execute(MessageRef message);
	
	void draw()
	{
		ofRect(rect);
	}
	
	void hittest()
	{
		ofRect(rect);
	}
	
	void addCord(PatchCord *cord)
	{
		cords.insert(cord);
	}
	
	void removeCord(PatchCord *cord)
	{
		cords.erase(cord);
	}
	
	void disconnectAll()
	{
		CordContainerType t = cords;
		CordContainerType::iterator it = t.begin();
		while (it != t.end())
		{
			PatchCord *c = *it;
			c->disconnect();
			it++;
		}
		cords.clear();
	}
	
	PortIdentifer::Direction getDirection() const { return direction; }
	
	void setRect(const ofRectangle& rect) { this->rect = rect; }
	const ofRectangle& getRect() { return rect; }
	
	ofVec3f getPos() const { return rect.getCenter(); }
	ofVec3f getGlobalPos() const;
	
	BasePatcher* getPatcher() const { return patcher; }
	
	bool hasConnectTo(Port *port);
	
protected:
	
	typedef std::set<PatchCord*> CordContainerType;
	CordContainerType cords;
	
	int index;
	BasePatcher *patcher;
	PortIdentifer::Direction direction;
	
	// data
	MessageRef data;
	
	ofRectangle rect;
};

class ofxInteractivePrimitives::BasePatcher : public ofxInteractivePrimitives::DelayedDeletable
{
	friend class Port;
	
public:
	
	virtual void execute() {}
	
	virtual Element2D* getUIElement() = 0;
	
	virtual int getNumInput() const { return 0; }
	virtual int getNumOutput() const { return 0; }
	
	virtual Port& getInputPort(int index) = 0;
	virtual Port& getOutputPort(int index) = 0;
	
	//
	
	virtual ofVec3f localToGlobalPos(const ofVec3f& v) = 0;
	virtual ofVec3f globalToLocalPos(const ofVec3f& v) = 0;
	virtual ofVec3f getPosition() = 0;
	
protected:
	
	virtual void inputDataUpdated(int index) = 0;
};



#pragma mark - Patcher

template <typename T, typename P = ofxInteractivePrimitives::NullParam, typename InteractivePrimitiveType = ofxInteractivePrimitives::DraggableStringBox>
class ofxInteractivePrimitives::Patcher : public BasePatcher, public InteractivePrimitiveType
{
public:
	
	P param;
	
	Patcher(Node &parent, const P& param = P()) : BasePatcher(), InteractivePrimitiveType(parent), param(param)
	{
		input_data.resize(getNumInput());
		output_data.resize(getNumOutput());
		
		content = (Context*)T::create(input_data, output_data);
		
		setupPatcher();
	}
	
	~Patcher()
	{
		this->dispose();
	}
	
	void dispose()
	{
		input_data.clear();
		output_data.clear();

		InteractivePrimitiveType::dispose();
	}
	
	int getNumInput() const { return T::getNumInput(); }
	TypeID getInputType(int index) const { return T::getInputType(index); }
	void setInput(int index, MessageRef data) {}
	
	int getNumOutput() const { return T::getNumOutput(); }
	TypeID getOutputType(int index) const { return T::getOutputType(index); }
	void setOutput(int index, MessageRef data) {}
	
	Port& getInputPort(int index) { return input_port.at(index); }
	Port& getOutputPort(int index) { return output_port.at(index); }
	
	void execute()
	{
		for (int i = 0; i < getNumInput(); i++)
		{
			Port &input_port = getInputPort(i);
			input_data[i] = input_port.data;
		}
		
		T::execute(this, content, input_data, output_data);
		
		for (int i = 0; i < getNumOutput(); i++)
		{
			Port &output_port = getOutputPort(i);
			output_port.execute(output_data[i]);
		}
	}
	
	// ofxIP
	
	void update() { InteractivePrimitiveType::update(); T::update(this, content); }
	
	void draw()
	{
		ofPushStyle();
		
		InteractivePrimitiveType::draw();
		
		{
			ofPushStyle();
			
			if (this->isHover())
				ofSetLineWidth(2);
			else
				ofSetLineWidth(1);
			
			if (this->isFocus())
				ofSetColor(ofColor::fromHex(0xCCFF77), 127);

			ofNoFill();
			
			ofRectangle r = this->getContentRect();
			r.x -= 2;
			r.y -= 2;
			r.width += 4;
			r.height += 4;
			
			ofRect(r);
			
			ofPopStyle();
		}
		
		const vector<GLuint>& names = this->getCurrentNameStack();
		if (this->isHover() && names.size() == 2)
		{
			int index = names[1];
			
			ofVec3f p;
			
			if (names[0] == PortIdentifer::INPUT
				&& in_range(names[1], 0, getNumInput()))
			{
				p = getInputPort(index).getPos();
				p.y -= 1;
			}
			else if (names[0] == PortIdentifer::OUTPUT
					 && in_range(names[1], 0, getNumOutput()))
			{
				p = getOutputPort(index).getPos();
			}
			else assert(false);
			
			ofRectangle r;
			r.setFromCenter(p, 14, 8);
			ofRect(r);
		}
		
		ofFill();
		
		for (int i = 0; i < getNumInput(); i++)
		{
			getInputPort(i).draw();
		}
		
		for (int i = 0; i < getNumOutput(); i++)
		{
			getOutputPort(i).draw();
		}
		
		if (patching_port && patching_port->getPatcher() == this)
		{
			ofLine(patching_port->getPos(), this->globalToLocalPos(ofVec2f(ofGetMouseX(), ofGetMouseY())));
		}
		
		ofPopStyle();
	}
	
	void hittest()
	{
		InteractivePrimitiveType::hittest();
		
		ofFill();
		
		// input
		this->pushID(PortIdentifer::INPUT);
		
		for (int i = 0; i < getNumInput(); i++)
		{
			this->pushID(i);
			getInputPort(i).hittest();
			this->popID();
		}
		
		this->popID();
		
		// output
		this->pushID(PortIdentifer::OUTPUT);
		
		for (int i = 0; i < getNumOutput(); i++)
		{
			this->pushID(i);
			getOutputPort(i).hittest();
			this->popID();
		}
		
		this->popID();
	}
	
	void mouseDragged(int x, int y, int button)
	{
		if (patching_port == 0)
		{
			InteractivePrimitiveType::mouseDragged(x, y, button);
		}
	}
	
	void mousePressed(int x, int y, int button)
	{
		const vector<GLuint>& names = this->getCurrentNameStack();
		
		if (names.size() == 2)
		{
			if (names[0] == PortIdentifer::INPUT)
			{
				patching_port = &getInputPort(names[1]);
			}
			else if (names[0] == PortIdentifer::OUTPUT)
			{
				patching_port = &getOutputPort(names[1]);
			}
			else assert(false);
		}
		else
		{
			InteractivePrimitiveType::mousePressed(x, y, button);
		}
	}
	
	void mouseReleased(int x, int y, int button)
	{
		if (patching_port)
		{
			const vector<GLuint>& names = this->getCurrentNameStack();
			
			if (names.size() == 2)
			{
				Port *upstream = NULL;
				Port *downstream = NULL;
				
				if (names[0] == PortIdentifer::OUTPUT
					&& in_range(names[1], 0, getNumInput()))
				{
					upstream = patching_port;
					downstream = &getInputPort(names[1]);
				}
				else if (names[0] == PortIdentifer::INPUT
						 && in_range(names[1], 0, getNumOutput()))
				{
					upstream = &getOutputPort(names[1]);
					downstream = patching_port;
				}
				else goto __cancel__;
				
				createPatchCord(upstream, downstream);
			}
		}
		else
		{
			InteractivePrimitiveType::mouseReleased(x, y, button);
		}
		
		__cancel__:;
		patching_port = NULL;
	}
	
	void keyPressed(int key)
	{
		if (key == OF_KEY_DEL || key == OF_KEY_BACKSPACE)
		{
			disposePatchCords();
			delayedDelete();
		}
		else
		{
			InteractivePrimitiveType::keyPressed(key);
		}
	}
	
	//
	
	ofVec3f localToGlobalPos(const ofVec3f& v) { return InteractivePrimitiveType::localToGlobalPos(v); }
	ofVec3f globalToLocalPos(const ofVec3f& v) { return InteractivePrimitiveType::globalToLocalPos(v); }
	ofVec3f getPosition() { return InteractivePrimitiveType::getPosition(); }
	
	Element2D* getUIElement() { return this; }
	
protected:
	
	void disposePatchCords()
	{
		struct disconnect
		{
			void operator()(Port &o)
			{
				o.disconnectAll();
			}
		};
		
		for_each(input_port.begin(), input_port.end(), disconnect());
		for_each(output_port.begin(), output_port.end(), disconnect());
	}
	
	void inputDataUpdated(int index)
	{
		execute();
	}
	
	void setupPatcher()
	{
		T::layout(this, content);
		
		disposePatchCords();
		input_port.clear();
		output_port.clear();
		
		for (int i = 0; i < getNumInput(); i++)
		{
			input_port.push_back(Port(this, i, PortIdentifer::INPUT));
		}
		
		for (int i = 0; i < getNumOutput(); i++)
		{
			output_port.push_back(Port(this, i, PortIdentifer::OUTPUT));
		}
		
		alignPort();
	}
	
	virtual void alignPort()
	{
		for (int i = 0; i < getNumInput(); i++)
		{
			ofRectangle rect;
			rect.x = 14 * i;
			rect.y = -5;
			rect.width = 10;
			rect.height = 4;
			
			getInputPort(i).setRect(rect);
		}
		
		for (int i = 0; i < getNumOutput(); i++)
		{
			ofRectangle rect;
			rect.x = 14 * i;
			rect.y = this->getContentHeight();
			rect.width = 10;
			rect.height = 4;
			
			getOutputPort(i).setRect(rect);
		}
	}
	
	PatchCord* createPatchCord(Port *upstream, Port *downstream)
	{
		// patching validation
		const char *msg = "unknown error";
		
		// port is null
		if (upstream == NULL || downstream == NULL)
		{
			msg = "port is null";
			goto __cancel__;
		}
		
		// already connected
		if (upstream->hasConnectTo(downstream))
		{
			msg = "already connected";
			goto __cancel__;
		}
		
		// patching oneself
		if (upstream->getPatcher() == downstream->getPatcher())
		{
			msg = "patching oneself";
			goto __cancel__;
		}
		
		// TODO: loop detection
		
		// create patchcord
		return new PatchCord(upstream, downstream);
		
	__cancel__:
		
		ofLogWarning("AbstructPatcher") << "patching failed: " << msg;
		return NULL;
	}
	
private:
	
	typedef typename T::Context Context;
	Context *content;
	
	vector<MessageRef> input_data, output_data;
	vector<Port> input_port, output_port;
};

#pragma mark - BaseWrapper

struct ofxInteractivePrimitives::BaseWrapper
{
	static void* create(vector<MessageRef>& input, vector<MessageRef>& output) { return NULL; }
	
	static void execute(BasePatcher *patcher, void *context, const vector<MessageRef>& input, vector<MessageRef>& output) {}
	
	static void layout(BasePatcher *patcher, void *context) {}
	static void update(BasePatcher *patcher, void *context) {}
	
	static int getNumInput()
	{
		return 0;
	}
	
	static TypeID getInputType(int index)
	{
		return Type2Int<void>();
	}
	
	static int getNumOutput()
	{
		return 0;
	}
	
	static TypeID getOutputType(int index)
	{
		return Type2Int<void>();
	}
};

template <
	typename T,
	typename ContextType = void,
	typename ParamType = ofxInteractivePrimitives::NullParam,
	typename InteractivePrimitiveType = ofxInteractivePrimitives::DraggableStringBox
>
struct ofxInteractivePrimitives::AbstructWrapper : public BaseWrapper
{
	typedef ContextType Context;
	typedef ParamType Param;
	typedef T Class;
	typedef ofxInteractivePrimitives::Patcher<Class, Param, InteractivePrimitiveType> Patcher;
	
	static void* create(vector<MessageRef>& input, vector<MessageRef>& output)
	{
		return NULL;
	}
	
	static void execute(Patcher *patcher, ContextType *context, const vector<MessageRef>& input, vector<MessageRef>& output) {}
	
	static void layout(Patcher *patcher, ContextType *context) {}
	static void update(Patcher *patcher, ContextType *context) {}
	
};
