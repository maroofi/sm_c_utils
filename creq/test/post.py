import cherrypy
import base64
import json

class post(object):
    def __init__(self):
        pass

    @cherrypy.expose
    def get_data(*args, **kwargs):
        print(cherrypy.request.method)
        print("params: {}".format(kwargs))
        if cherrypy.request.method == "GET":
            return "GET method received"
        else:
            ln = cherrypy.request.headers.get("Content-Length", None) or None
            ln = int(ln)
            print("Length of data is: {}".format(ln))
            body = cherrypy.request.body.read(ln)
            if isinstance(body, bytes):
                body = body.decode()
            print("body is: {}".format(body))
            print(json.loads(body))
            return "done"
    # end def
# end class

if __name__ == "__main__":
    cherrypy.quickstart(post())
