#pragma once

#include "ofMain.h"

#include <assert.h>

namespace ofxInteractivePrimitives
{
	class Context;
	class Node;
	class RootNode;
}

class ofxInteractivePrimitives::Node : public ofNode
{
	friend class RootNode;

public:

	friend class Context;

	Node();
	virtual ~Node();
	
	virtual void dispose();

	virtual void update() {}
	virtual void draw() {}
	virtual void hittest() {}

	virtual void mousePressed(int x, int y, int button)
	{
		if (hasParent())
		{
			ofVec3f g = localToGlobalPos(ofVec3f(x, y, 0));
			g = getParent()->globalToLocalPos(g);
			getParent()->mousePressed(g.x, g.y, button);
		}
	}
	
	virtual void mouseReleased(int x, int y, int button)
	{
		if (hasParent())
		{
			ofVec3f g = localToGlobalPos(ofVec3f(x, y, 0));
			g = getParent()->globalToLocalPos(g);
			getParent()->mouseReleased(g.x, g.y, button);
		}
	}
	
	virtual void mouseMoved(int x, int y)
	{
		if (hasParent())
		{
			ofVec3f g = localToGlobalPos(ofVec3f(x, y, 0));
			g = getParent()->globalToLocalPos(g);
			getParent()->mouseMoved(g.x, g.y);
		}
	}
	
	virtual void mouseDragged(int x, int y, int button)
	{
		if (hasParent())
		{
			ofVec3f g = localToGlobalPos(ofVec3f(x, y, 0));
			g = getParent()->globalToLocalPos(g);
			getParent()->mouseDragged(g.x, g.y, button);
		}
	}

	virtual void keyPressed(int key) {}
	virtual void keyReleased(int key) {}

	//

	void setParent(Node *o);
	Node* getParent() { return (Node*)ofNode::getParent(); }
	bool hasParent() { return ofNode::getParent() != NULL; }
	void clearParent();
	
	vector<Node*> getChildren() { return children; }
	
	//

	inline void setVisible(bool v) { visible = v; }
	inline bool getVisible() const { return visible; }
	inline bool isVisible() const { return visible; }

	inline bool isHover() const { return hover; }
	inline bool isDown() const { return down; }

	inline bool isFocus() const { return focus; }
	inline bool hasFocus() const { return focus; }
	
	inline void setEnable(bool v) { enable = v; }
	inline bool getEnable() { return enable; }
	inline bool isEnable() { return enable; }
	

	// utils

	ofVec2f getMouseDelta();
	ofVec3f localToGlobalPos(const ofVec3f& v);
	ofVec3f globalToLocalPos(const ofVec3f& v);

	ofVec3f screenToWorld(const ofVec2f& v);
	ofVec2f worldToScreen(const ofVec3f& v);

protected:

	struct Internal {};
	void draw(const Internal &);
	void update(const Internal &);

	virtual Context* getContext();
	const vector<GLuint>& getCurrentNameStack();
	
	inline void pushID(int id) { glPushName(id); }
	inline void popID() { glPopName(); }

	void cancelFocus();
	
private:

	unsigned int object_id;
	bool hover, down, focus, visible, enable;

	ofMatrix4x4 global_matrix, global_matrix_inverse;
	vector<Node*> children;

	void clearState();

};

class ofxInteractivePrimitives::RootNode : public ofxInteractivePrimitives::Node
{
public:

	RootNode();
	~RootNode();

	void draw();
	void update();
	
	bool hasFocusdObject();
	
	void enableAllEvent();
	void disableAllEvent();

protected:

	Context* getContext();

private:

	Context *context;
};


// primitives

#include "ui/ofxIPBaseElement.h"
#include "ui/ofxIPMarker.h"
#include "ui/ofxIPSlider.h"
#include "ui/ofxIPButton.h"
#include "ui/ofxIPPatcher.h"
#include "ui/ofxIPStringBox.h"
